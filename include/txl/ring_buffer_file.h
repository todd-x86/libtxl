#pragma once

#include <txl/buffer_ref.h>
#include <txl/memory_map.h>
#include <txl/result.h>
#include <txl/file.h>
#include <txl/types.h>
#include <txl/system_error.h>

#include <algorithm>
#include <iostream>
#include <atomic>
#include <string>
#include <string_view>
#include <cstddef>

namespace txl
{
    class ring_buffer_file final
    {
    public:
        struct cursor_data;
        struct file_cursor_data;

        friend auto operator<<(std::ostream & os, cursor_data const & c) -> std::ostream &;
        friend auto operator<<(std::ostream & os, file_cursor_data const & c) -> std::ostream &;
        
        struct cursor_data final
        {
            uint64_t offset_;

            auto distance(cursor_data const & c) const -> int64_t
            {
                return c.offset_ - offset_;
            }

            auto inc(size_t s) -> void
            {
                offset_ += s;
            }

            auto operator<(cursor_data const & c) const -> bool
            {
                return offset_ < c.offset_;
            }

            auto operator==(cursor_data const & c) const -> bool
            {
                return not (*this < c) and not (c < *this);
            }

            auto operator>(cursor_data const & c) const -> bool
            {
                return c < *this;
            }

            auto operator>=(cursor_data const & c) const -> bool
            {
                return (*this > c) or (*this == c);
            }
        };

        struct file_cursor_data final
        {
            cursor_data head_;
            char pad1_[64];
            cursor_data tail_;
            char pad2_[64];
        };
    private:
        struct entry_data final
        {
            uint32_t size_;
            std::byte data_[0];
        };

        file storage_;
        memory_map map_;
        size_t max_size_;
        size_t ring_padding_;
        file_cursor_data cur_{{0}, {0}};

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

        auto cursor_entry(cursor_data const & c) const -> entry_data *
        {
            auto p = c.offset_ % entry_map().size();
            return entry_at(p);
        }

        auto tail_to_end() const -> buffer_ref
        {
            return entry_map().slice(cur_.tail_.offset_ % entry_map().size());
        }

        auto head_to_end() const -> buffer_ref
        {
            return entry_map().slice(cur_.head_.offset_ % entry_map().size());
        }

        static auto total_entry_size(size_t bytes) -> uint64_t
        {
            return sizeof(entry_data) + bytes;
        }
    public:
        enum open_mode
        {
            read_only,
            read_write,
        };

        ring_buffer_file(size_t max_size)
            : ring_buffer_file(max_size, max_size >> 1)
        {
        }

        ring_buffer_file(size_t max_size, size_t ring_size)
            : max_size_(max_size)
            , ring_padding_(max_size - std::min(max_size, ring_size))
        {
        }
        
        ring_buffer_file(std::string const & filename, open_mode mode, size_t max_size)
            : ring_buffer_file(filename, mode, max_size, max_size >> 1)
        {
        }

        ring_buffer_file(std::string const & filename, open_mode mode, size_t max_size, size_t ring_size)
            : ring_buffer_file(max_size, ring_size)
        {
            open(filename, mode).or_throw();
        }

        auto open(std::string const & filename, open_mode mode) -> result<void>
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
            auto f_cur = *file_cursor();
            if (f_cur.head_ > cur_.head_)
            {
                // Update our out-of-date cursor
                cur_ = f_cur;
            }

            if (cur_.head_ >= f_cur.tail_)
            {
                // Don't advance past tail
                return buffer_ref{};
            }

            auto * e = cursor_entry(cur_.head_);
            if (e == nullptr or e->size_ == 0)
            {
                // Loop around
                cur_.head_.inc(head_to_end().size());
                
                if (cur_.head_ >= f_cur.tail_)
                {
                    // Don't advance past tail
                    return buffer_ref{};
                }
                
                e = cursor_entry(cur_.head_);
            }
            
            // Move head forward
            cur_.head_.inc(total_entry_size(e->size_));
            return buffer_ref{&e->data_, e->size_};
        }

        auto write(buffer_ref src) -> result<buffer_ref>
        {
            auto bytes_needed = total_entry_size(src.size());
            auto tte = tail_to_end();
            buffer_ref dst{};

            auto extra_bytes_needed = 0;
            if (tte.size() < bytes_needed)
            {
                extra_bytes_needed = tte.size();
            }

            // Keep head in sync w/ tail
            while (cur_.head_.distance(cur_.tail_) + extra_bytes_needed + bytes_needed + ring_padding_ >= entry_map().size())
            {
                auto * e = cursor_entry(cur_.head_);
                if (e == nullptr or e->size_ == 0)
                {
                    // Skip zero portion
                    head_to_end().fill(static_cast<std::byte>('\0'));
                    cur_.head_.inc(head_to_end().size());
                    e = cursor_entry(cur_.head_);
                }

                // Move head forward
                auto s = total_entry_size(e->size_);
                
                // Mark it empty
                e->size_ = 0;
                cur_.head_.inc(s);
            }

            if (tte.size() >= bytes_needed)
            {
                auto * e = cursor_entry(cur_.tail_);
                
                dst = buffer_ref(&e->data_, src.size());
                dst.copy_from(src);
                e->size_ = src.size();
            }
            else if (tte.size() >= sizeof(entry_data))
            {
                // Stamp 0 and move back to 0
                cursor_entry(cur_.tail_)->size_ = 0;

                cur_.tail_.inc(tte.size());
                
                auto * e = cursor_entry(cur_.tail_);

                dst = buffer_ref(&e->data_, src.size());
                dst.copy_from(src);
                e->size_ = src.size();
            }
            else
            {
                // Just move to the end and go back
                cur_.tail_.inc(tte.size());
                
                auto * e = cursor_entry(cur_.tail_);

                dst = buffer_ref(&e->data_, src.size());
                dst.copy_from(src);
                e->size_ = src.size();
            }

            cur_.tail_.inc(bytes_needed);
            *file_cursor() = cur_;
            return dst;
        }

        auto cursor_internal() const -> file_cursor_data { return cur_; }
        auto cursor_file() const -> file_cursor_data { return *file_cursor(); }
    };

    inline auto operator<<(std::ostream & os, ring_buffer_file::cursor_data const & c) -> std::ostream &
    {
        os << "(O=" << c.offset_ << ")";
        return os;
    }
    inline auto operator<<(std::ostream & os, ring_buffer_file::file_cursor_data const & c) -> std::ostream &
    {
        os << "H=" << c.head_ << " | T=" << c.tail_ << " (d=" << c.head_.distance(c.tail_) << ")";
        return os;
    }
}
