#pragma once

#include <txl/buffer_ref.h>
#include <txl/result.h>
#include <txl/file.h>
#include <txl/types.h>
#include <txl/system_error.h>

#include <string_view>
#include <cstddef>
#include <atomic>

namespace txl
{
    /**
     * [size:4]...[0000]  -- write a 0 to signal end of buffer
     * no partial wrap-around allowed
     */
    class ring_buffer_file
    {
    private:
        file storage_;
        std::atomic<off_t> next_off_;
        size_t max_size_;
    public:
        enum open_mode
        {
            read_only,
            read_write,
        };

        ring_buffer_file(size_t max_size)
            : max_size_(max_size)
        {
            next_off_.store(0, std::memory_order_release);
        }

        ring_buffer_file(std::string_view filename, open_mode mode, size_t max_size)
            : ring_buffer_file(max_size)
        {
            open(filename, mode).or_throw();
        }

        auto open(std::string_view filename, open_mode mode) -> result<void>
        {
            auto str_mode = "w+";
            switch (mode)
            {
                case read_only:
                    str_mode = "r";
                    break;
                case read_write:
                    str_mode = "w+";
                    break;
            }
            return storage_.open(filename, str_mode);
        }
        
        auto fd() const -> int { return storage_.fd(); }

        auto is_open() const -> bool { return storage_.is_open(); }

        auto close() -> result<void> { return storage_.close(); }

        auto read_into(byte_vector & dst) -> result<buffer_ref>
        {
            auto before_size = dst.size();
            
            uint64_t segment_size = 0;
            auto segment_size_br = buffer_ref::cast(segment_size);

            auto read_at = next_off_.load(std::memory_order_acquire);
            if (auto res = storage_.read(read_at, segment_size_br); res.is_error())
            {
                return res;
            }

            // Do not advance
            if (segment_size == 0)
            {
                return {buffer_ref{}};
            }
            next_off_.store(read_at + sizeof(segment_size) + segment_size, std::memory_order_release);

            dst.resize(before_size + segment_size);
            read_at += sizeof(segment_size);

            return storage_.read(read_at, buffer_ref{&dst[before_size], dst.size() - before_size});
        }

        auto write(buffer_ref src) -> result<buffer_ref>
        {
            const uint64_t ZERO = 0;

            uint64_t segment_size = src.size();
            auto bytes_to_advance = segment_size + sizeof(segment_size);

            if (bytes_to_advance > max_size_)
            {
                // Can't write bigger than the file allows
                return {get_system_error(EBADF)};
            }

            auto write_at = next_off_.load(std::memory_order_acquire);
            if (write_at + bytes_to_advance <= max_size_)
            {
                // Increment
                next_off_.store(write_at + bytes_to_advance, std::memory_order_release);
            }
            else
            {
                // Start at next offset
                write_at = 0;
                next_off_.store(bytes_to_advance, std::memory_order_release);
            }

            auto write_segment_size_at = write_at;

            write_at += sizeof(segment_size);

            if (auto res = storage_.write(write_at, src); res.is_error() or static_cast<size_t>(write_at) == max_size_)
            {
                return res;
            }
            write_at += src.size();

            // Append zero to signify last entry
            if (auto res = storage_.write(write_at, buffer_ref::cast(ZERO)); res.is_error())
            {
                return res;
            }
            
            // Go back and write the segment size so it's readable
            return storage_.write(write_segment_size_at, buffer_ref::cast(segment_size));
        }
    };
}
