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
