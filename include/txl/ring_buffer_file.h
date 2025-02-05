#pragma once

#include <txl/buffer_ref.h>
#include <txl/memory_map.h>
#include <txl/result.h>
#include <txl/file.h>
#include <txl/types.h>
#include <txl/system_error.h>

#include <iostream>
#include <atomic>
#include <string_view>
#include <cstddef>

namespace txl
{
    /**
     * 0x00: [Head:8][Tail:8]
     * 0x10: [Cycle:8][EntrySize:4][Data...]...
     */
    class ring_buffer_file final
    {
    public:
        struct cursor_data;
        struct file_cursor_data;

        friend auto operator<<(std::ostream & os, cursor_data const & c) -> std::ostream &;
        friend auto operator<<(std::ostream & os, file_cursor_data const & c) -> std::ostream &;

        struct cursor_data
        {
            uint64_t offset_;
            uint64_t cycle_;

            auto distance_from(cursor_data const & c, size_t max_size) const -> off_t
            {
                return ((c.cycle_ - cycle_) * max_size) + (c.offset_ - offset_);
            }

            auto operator<(cursor_data const & c) const -> bool
            {
                return cycle_ < c.cycle_ or (cycle_ == c.cycle_ and offset_ < c.offset_);
            }
            
            auto operator<=(cursor_data const & c) const -> bool
            {
                return cycle_ < c.cycle_ or (cycle_ == c.cycle_ and offset_ <= c.offset_);
            }

            auto next(uint64_t offset, uint64_t max_size) -> cursor_data
            {
                auto c = *this;
                c.offset_ += offset;
                c.cycle_ += (c.offset_ / max_size);
                c.offset_ = (c.offset_ % max_size);
                return c;
            }

            auto fetch(cursor_data & c) -> void
            {
                __atomic_load(&c.offset_, &offset_, static_cast<int>(std::memory_order_seq_cst));
                __atomic_load(&c.cycle_, &cycle_, static_cast<int>(std::memory_order_seq_cst));
            }

            auto sync(cursor_data const & c) -> void
            {
                __atomic_store(&offset_, &c.offset_, static_cast<int>(std::memory_order_seq_cst));
                __atomic_store(&cycle_, &c.cycle_, static_cast<int>(std::memory_order_seq_cst));
            }
        };

        struct file_cursor_data
        {
            cursor_data head_;
            cursor_data tail_;

            auto fetch(file_cursor_data & c) -> void
            {
                head_.fetch(c.head_);
                tail_.fetch(c.tail_);
            }

            auto sync(file_cursor_data const & c) -> void
            {
                head_.sync(c.head_);
                tail_.sync(c.tail_);
            }
        };
    private:
        struct entry_data
        {
            uint32_t size_;
            std::byte data_[0];
        };

        file storage_;
        memory_map map_;
        size_t max_size_;
        file_cursor_data cur_{{0, 0}, {0, 0}};

        auto file_cursor() const -> file_cursor_data *
        {
            return map_.memory().to_alias<file_cursor_data>();
        }
        
        auto entry_map() const -> buffer_ref
        {
            return map_.memory().slice(sizeof(file_cursor_data));
        }

        auto entry_at(size_t offset) const -> entry_data *
        {
            return entry_map().slice(offset).to_alias<entry_data>();
        }

        auto head_entry() const -> entry_data *
        {
            return entry_at(cur_.head_.offset_);
        }

        auto tail_entry() const -> entry_data *
        {
            return entry_at(cur_.tail_.offset_);
        }

        auto next_head() -> void
        {
            auto * e = head_entry();
            auto total_bytes = sizeof(entry_data) + e->size_;
            cur_.head_.offset_ += total_bytes;
            if (cur_.head_.offset_ + sizeof(entry_data) > entry_map().size())
            {
                // Loop around
                ++cur_.head_.cycle_;
                cur_.head_.offset_ = 0;
            }
        }

        auto next_tail() -> void
        {
            auto * e = tail_entry();
            cur_.tail_.offset_ += sizeof(entry_data) + e->size_;
        }
    public:
        enum open_mode
        {
            read_only,
            read_write,
        };

        ring_buffer_file(size_t max_size)
            : max_size_(max_size)
        {
        }

        ring_buffer_file(std::string_view filename, open_mode mode, size_t max_size)
            : ring_buffer_file(max_size)
        {
            open(filename, mode).or_throw();
        }

        auto open(std::string_view filename, open_mode mode) -> result<void>
        {
            auto str_mode = "w+";
            auto mm_mode = memory_map::read | memory_map::write;
            switch (mode)
            {
                case read_only:
                    str_mode = "r";
                    mm_mode = memory_map::read;
                    break;
                case read_write:
                    str_mode = "w+";
                    mm_mode = memory_map::read | memory_map::write;
                    break;
            }
            auto res = storage_.open(filename, str_mode);
            if (mode != read_only)
            {
                res.then([this]() {
                    return storage_.truncate(max_size_);
                });
            }
            return res.then([this, mm_mode]() {
                return map_.open(max_size_, mm_mode, true, memory_map::no_swap, nullptr, storage_.fd());
            }).then([this]() {
                cur_ = *file_cursor();
                return result<void>{};
            });
        }
        
        auto fd() const -> int { return storage_.fd(); }

        auto is_open() const -> bool { return storage_.is_open() or map_.is_open(); }

        auto close() -> result<void>
        {
            return map_.close()
                .then([this]() {
                    return storage_.close();
                });
        }

        auto read() -> result<buffer_ref>
        {
            auto * e = head_entry();
            file_cursor_data f_cur{};
            f_cur.fetch(*file_cursor());
            if (cur_.head_ < f_cur.head_)
            {
                cur_ = f_cur; //*file_cursor();
            }
            
            // Advance if we have room
            if (cur_.head_ < f_cur.tail_ and cur_.head_.next(e->size_, entry_map().size()) <= f_cur.tail_)
            {
                next_head();
            }

            if (f_cur.tail_ <= cur_.head_)
            {
                // End-of-file
                return buffer_ref{};
            }
            return buffer_ref{&e->data_, e->size_};
        }

        auto write(buffer_ref src) -> result<buffer_ref>
        {
            auto bytes_needed = sizeof(entry_data) + src.size();
            auto dst = entry_map().slice_n(cur_.tail_.offset_, bytes_needed);
            if (dst.size() < bytes_needed)
            {
                // Loop around
                cur_.tail_.offset_ = 0;
                ++cur_.tail_.cycle_;
            }

            // Keep the head moving forward to fit new entries
            while (cur_.head_.distance_from(cur_.tail_, entry_map().size()) >= static_cast<off_t>(entry_map().size() - bytes_needed))
            {
                next_head();
            }

            auto * e = tail_entry();
            dst = buffer_ref{&e->data_, src.size()};
            dst.copy_from(src);
            __atomic_store_n(&e->size_ , src.size(), static_cast<int>(std::memory_order_seq_cst));
            
            next_tail();

            // Stamp the next entry 0 so read() will stop gracefully
            if (cur_.tail_.offset_ + sizeof(entry_data) <= entry_map().size())
            {
                __atomic_store_n(&tail_entry()->size_, 0, static_cast<int>(std::memory_order_release));
            }
            file_cursor()->sync(cur_);
            
            return dst;
        }

        auto cursor_internal() const -> file_cursor_data { return cur_; }
        auto cursor_file() const -> file_cursor_data { return *file_cursor(); }
    };

    inline auto operator<<(std::ostream & os, ring_buffer_file::cursor_data const & c) -> std::ostream &
    {
        os << "(O=" << c.offset_ << ", C=" << c.cycle_ << ")";
        return os;
    }
    inline auto operator<<(std::ostream & os, ring_buffer_file::file_cursor_data const & c) -> std::ostream &
    {
        os << "H=" << c.head_ << " | T=" << c.tail_;
        return os;
    }
}
