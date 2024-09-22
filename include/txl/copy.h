#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/on_error.h>
#include <txl/system_error.h>

#include <cstdlib>
#include <ostream>

namespace txl
{
    auto copy(reader & src, std::ostream & dst, buffer_ref copy_buf, size_t bytes_to_read, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> size_t
    {
        size_t total_read = 0;
        while (total_read < bytes_to_read)
        {
            auto buf_read = src.read(copy_buf, on_err);
            if (buf_read.empty())
            {
                break;
            }
	    // TODO: check dst.good()
            dst.write(reinterpret_cast<char const *>(copy_buf.data()), buf_read.size());
            total_read += buf_read.size();
        }

        return total_read;
    }
    
    auto copy(reader & src, std::ostream & dst, buffer_ref copy_buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> size_t
    {
        return copy(src, dst, copy_buf, copy_buf.size(), on_err);
    }
}
