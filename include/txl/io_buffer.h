#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/result.h>
#include <txl/types.h>

#include <algorithm>
#include <cstddef>

namespace txl
{
    class io_buffer : public reader
                    , public writer
    {
    private:
        byte_vector buf_{};
    protected:
        auto read_impl(buffer_ref dst) -> result<size_t> override
        {
            auto src = buffer_ref{buf_.data(), buf_.size()};
            auto num_copied = dst.copy_from(src);
            // TODO: better container???
            buf_.erase(buf_.begin(), std::next(buf_.begin(), num_copied));
            return num_copied;
        }
        
        auto write_impl(buffer_ref src) -> result<size_t> override
        {
            buf_.reserve(buf_.capacity() + src.size());
            auto b = buf_.end();
            auto e = std::copy(src.begin(), src.end(), std::back_inserter(buf_));
            return std::distance(b, e);
        }
    };
}
