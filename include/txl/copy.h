#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/on_error.h>
#include <txl/system_error.h>

#include <algorithm>
#include <cstdlib>
#include <type_traits>

namespace txl
{
    struct size_policy
    {
        size_t value;

        size_policy(size_t i)
            : value(i)
        {
        }

        auto process(size_t requested, size_t actual) -> void
        {
            if (actual >= value)
            {
                value = 0;
            }
            else
            {
                value -= actual;
            }
        }

        auto is_complete() const -> bool
        {
            return value == 0;
        }
    };

    struct exactly final : size_policy
    {
    };
    
    struct at_most final : size_policy
    {
        bool maybe_eof_ = false;

        auto process(size_t requested, size_t actual) -> void
        {
            maybe_eof_ = (requested > actual);
            size_policy::process(requested, actual);
        }

        auto is_complete() const -> bool
        {
            return value == 0 or maybe_eof_;
        }
    };

    template<class SizePolicy>
    auto copy(reader & src, writer & dst, buffer_ref copy_buf, SizePolicy bytes_to_read, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> size_t
    {
        size_t total_read = 0;
        while (not bytes_to_read.is_complete())
        {
            auto input_buf = copy_buf.slice(0, bytes_to_read.value);
            auto buf_read = src.read(input_buf, on_err);
            if (buf_read.empty())
            {
                break;
            }

            auto written = dst.write(buf_read, on_err);
            total_read += written.size();
            bytes_to_read.process(input_buf.size(), written.size());
        }

        return total_read;
    }
    
    template<class SizePolicy, class = std::enable_if_t<std::is_base_of_v<size_policy, SizePolicy>>>
    inline auto copy(reader & src, writer & dst, SizePolicy bytes_to_read, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> size_t
    {
        // Allocate a temporary copy-buffer on the stack (4K at max)
        auto buf_size = std::min(static_cast<size_t>(bytes_to_read.value), static_cast<size_t>(4096));
        std::byte buf[buf_size];
        
        return copy(src, dst, buffer_ref{buf, buf_size}, bytes_to_read, on_err);
    }
    
    inline auto copy(reader & src, writer & dst, buffer_ref copy_buf, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> size_t
    {
        return copy(src, dst, copy_buf, exactly{copy_buf.size()}, on_err);
    }
}
