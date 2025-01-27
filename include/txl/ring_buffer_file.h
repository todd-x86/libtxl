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
            uint64_t head_;
            uint64_t tail_;
            uint64_t cycle_;
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
            return entry_map().slice(sizeof(header_data) + offset).to_alias<entry_data>();
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
            auto * e = entry_at(offset_);
            
            // Do not advance
            if (e->size_ == 0)
            {
                return {buffer_ref{}};
            }

            if (offset_ + sizeof(entry_data) + e->size_ > max_size_)
            {
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
            const entry_data SENTINEL = { .size_ = 0 };
            if (src.size() + sizeof(header_data) + sizeof(entry_data) > max_size_)
            {
                // Can't write bigger than the memory mapped file allows
                return {get_system_error(EINVAL)};
            }

            auto offset = offset_;
            if (offset + sizeof(header_data) + src.size() > max_size_)
            {
                // Signify the end
                entry_at(offset)->size_ = 0;

                // Loop back to beginning
                offset = 0;
            }
            auto * e = entry_at(offset);
            e->size_ = src.size();
            auto dst = buffer_ref{e->data_, e->size_};
            auto bytes_copied = dst.copy_from(src);

            // Stamp a zero to signify the end
            *entry_at(offset + sizeof(entry_data) + e->size_) = SENTINEL;
            offset += sizeof(entry_data) + e->size_;
            offset_ = header_map()->tail_ = offset;

            return src.slice(0, bytes_copied);
        }
    };
}
