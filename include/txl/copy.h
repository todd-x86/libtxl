#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/result.h>
#include <txl/size_policy.h>
#include <txl/system_error.h>

#include <algorithm>
#include <cstdlib>
#include <type_traits>

namespace txl
{
    /**
     * Copies a series of bytes from a reader into a writer using a buffer ref as the temporary copy buffer and a size policy for determining how many bytes will be read. The size policy dictates that the number of bytes does not always need to be a fixed number but may conform to a more sophisticated approach when copying.
     */
    template<class SizePolicy, class = std::enable_if_t<std::is_base_of_v<size_policy, SizePolicy>>>
    auto copy(reader & src, writer & dst, buffer_ref copy_buf, SizePolicy bytes_to_read) -> result<size_t>
    {
        size_t total_read = 0;
        while (not bytes_to_read.is_complete())
        {
            auto input_buf = copy_buf.slice(0, bytes_to_read.value());
            auto buf_read = src.read(input_buf);
            if (not buf_read)
            {
                return buf_read.error();
            }
            if (buf_read->empty())
            {
                break;
            }

            auto written = dst.write(*buf_read);
            if (not written)
            {
                return written.error();
            }
            total_read += written->size();
            bytes_to_read.process(input_buf, *written);
        }

        return total_read;
    }
   
    /**
     * Copy is a series of bytes from a reader into a writer with a temporary buffer of four kilobytes and a size policy directing how many bytes to copy.
     */
    template<class SizePolicy, class = std::enable_if_t<std::is_base_of_v<size_policy, SizePolicy>>>
    inline auto copy(reader & src, writer & dst, SizePolicy bytes_to_read) -> result<size_t>
    {
        // Allocate a temporary copy-buffer on the stack (4K at max)
        std::byte buf[4096];
        
        return copy(src, dst, buffer_ref{buf}, bytes_to_read);
    }
   
    /**
     * Copies a series of bytes from a reader into a writer with a temporary buffer ref for storing the copied information during transport . The size policy conforms to the size of the copy buffer . 
     */
    inline auto copy(reader & src, writer & dst, buffer_ref copy_buf) -> result<size_t>
    {
        return copy(src, dst, copy_buf, exactly{copy_buf.size()});
    }
}
