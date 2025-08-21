#pragma once

#include <txl/buffer_ref.h>
#include <txl/result.h>

namespace txl
{
    struct reader
    {
    protected:
        // Returns buffer read
        virtual auto read_impl(buffer_ref buf) -> result<size_t> = 0;
    public:
        auto read(buffer_ref buf) -> result<buffer_ref>
        {
            auto bytes_read = read_impl(buf);
            if (not bytes_read)
            {
                return bytes_read.error();
            }
            return buf.slice(0, *bytes_read);
        }
    };
    
    struct position_reader
    {
    protected:
        // Returns buffer read
        virtual auto read_impl(off_t offset, buffer_ref buf) -> result<size_t> = 0;
    public:
        auto read(off_t offset, buffer_ref buf) -> result<buffer_ref>
        {
            auto bytes_read = read_impl(offset, buf);
            if (not bytes_read)
            {
                return bytes_read.error();
            }
            return buf.slice(0, *bytes_read);
        }
    };
    
    struct writer
    {
    protected:
        // Returns buffer written
        virtual auto write_impl(buffer_ref buf) -> result<size_t> = 0;
    public:
        auto write(buffer_ref buf) -> result<buffer_ref>
        {
            auto bytes_written = write_impl(buf);
            if (not bytes_written)
            {
                return bytes_written.error();
            }
            return buf.slice(0, *bytes_written);
        }
    };
}
