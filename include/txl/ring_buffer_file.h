#pragma once

#include <txl/buffer_ref.h>
#include <txl/memory_map.h>
#include <txl/result.h>
#include <txl/file.h>
#include <txl/types.h>
#include <txl/system_error.h>

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
    private:
        struct header_data
        {
            uint64_t head_; // byte offset
            uint64_t tail_; // byte offset
            uint64_t cycle_; // number of cycles around the file (for iterator synchronizing)
        };

        struct entry_data
        {
            uint32_t size_;
            std::byte data_[0];
        };

        file storage_;
        memory_map map_;
        size_t max_size_;
        size_t offset_ = 0;
        uint64_t cycle_ = 0;

        auto header_map() const -> header_data *
        {
            return map_.memory().to_alias<header_data>();
        }
        
        auto entry_map() const -> buffer_ref
        {
            return map_.memory().slice(sizeof(header_data));
        }

        auto entry_at(size_t offset) const -> entry_data *
        {
            return entry_map().slice(offset).to_alias<entry_data>();
        }

        auto should_advance_head(size_t size) const -> bool
        {
            size_t span;
            auto head = header_map()->head_;
            auto tail = header_map()->tail_;

            auto bytes_available = entry_map().size();
            auto bytes_needed = size + sizeof(entry_data);
            if (tail >= head)
            {
                span = std::max(bytes_available - tail, head);
            }
            else
            {
                span = (head - tail);
            }

            return span < bytes_needed;
        }

        auto advance_head() -> void
        {
            auto * e = entry_at(header_map()->head_);
            if (e->size_ == 0)
            {
                return;
            }

            auto next_head = header_map()->head_ + e->size_ + sizeof(entry_data);
            if (next_head + sizeof(entry_data) >= entry_map().size())
            {
                next_head = 0;
            }

            header_map()->head_ = next_head;
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

        auto offset() const -> size_t { return offset_; }
        auto file_head() const -> uint64_t { return header_map()->head_; }
        auto file_tail() const -> uint64_t { return header_map()->tail_; }

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
                offset_ = header_map()->head_;
                cycle_ = header_map()->cycle_;
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
            if (cycle_ != header_map()->cycle_)
            {
                offset_ = header_map()->head_;
                cycle_ = header_map()->cycle_;
            }

            auto * e = entry_at(offset_);
            
            // Do not advance
            if (e->size_ == 0)
            {
                return {buffer_ref{}};
            }

            if (offset_ + sizeof(entry_data) + e->size_ > max_size_)
            {
                ++cycle_;
                offset_ = 0;
            }
            else
            {
                offset_ += sizeof(entry_data) + e->size_;
            }

            return {buffer_ref{e->data_, e->size_}};
        }

        auto write(buffer_ref src) -> result<buffer_ref>
        {
            if (src.size() + sizeof(header_data) + sizeof(entry_data) > max_size_)
            {
                // Can't write bigger than the memory mapped file allows
                return {get_system_error(EINVAL)};
            }
            
            while (should_advance_head(src.size()))
            {
                advance_head();
            }

            auto offset = offset_;
            if (offset + sizeof(header_data) + sizeof(entry_data) + src.size() > max_size_)
            {
                // Signify the end
                entry_at(offset)->size_ = 0;

                // Loop back to beginning
                offset = 0;
                ++cycle_;
                header_map()->cycle_ = cycle_;
            }

            auto * e = entry_at(offset);
            e->size_ = src.size();
            auto dst = buffer_ref{e->data_, e->size_};
            auto bytes_copied = dst.copy_from(src);

            offset += sizeof(entry_data) + e->size_;
            offset_ = header_map()->tail_ = offset;

            return src.slice(0, bytes_copied);
        }
    };
}
