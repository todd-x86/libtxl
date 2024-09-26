#pragma once

#include <txl/buffer_ref.h>
#include <txl/system_error.h>
#include <txl/on_error.h>

namespace txl
{
    struct reader
    {
    protected:
        // Returns buffer read
        virtual auto read_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t = 0;
    public:
        auto read(buffer_ref buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> buffer_ref
        {
            auto bytes_read = read_impl(buf, on_err);
            return buf.slice(0, bytes_read);
        }
    };
    
    struct writer
    {
    protected:
        // Returns buffer written
        virtual auto write_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t = 0;
    public:
        auto write(buffer_ref buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> buffer_ref
        {
            auto bytes_written = write_impl(buf, on_err);
            return buf.slice(0, bytes_written);
        }
    };
}
