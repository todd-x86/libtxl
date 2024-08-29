#pragma once

#include <txl/buffer_ref.h>
#include <txl/system_error.h>
#include <txl/types.h>
#include <txl/on_error.h>

#include <iostream>
#include <algorithm>

namespace txl
{
    struct reader
    {
    protected:
        // Returns buffer read
        virtual auto read_impl(buffer_ref & buf, on_error::callback<system_error> on_err) -> buffer_ref = 0;
    public:
        auto read(buffer_ref & buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> buffer_ref
        {
            return read_impl(buf, on_err);
        }

        auto read(buffer_ref && buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> buffer_ref
        {
            return read_impl(buf, on_err);
        }

        auto read(std::ostream & dst, size_t size, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> size_t
        {
            char buf[size];
            auto rd_buf = buffer_ref{buf, size};

            size_t total_read = 0;
            while (total_read < size)
            {
                auto buf_read = read(rd_buf, on_err);
                if (buf_read.empty())
                {
                    break;
                }
                dst.write(reinterpret_cast<char const *>(rd_buf.data()), buf_read.size());
                total_read += buf_read.size();
            }

            return total_read;
        }
    };
    
    struct writer
    {
    protected:
        // Returns buffer written
        virtual auto write_impl(buffer_ref & buf, on_error::callback<system_error> on_err) -> buffer_ref = 0;
    public:
        auto write(buffer_ref & buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> buffer_ref
        {
            return write_impl(buf, on_err);
        }

        auto write(buffer_ref const & buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> buffer_ref
        {
            auto buf_copy = buffer_ref{buf};
            return write_impl(buf_copy, on_err);
        }
    };
}
