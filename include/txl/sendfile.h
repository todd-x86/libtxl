#pragma once

#include <txl/file_base.h>
#include <txl/patterns.h>
#include <txl/result.h>
#include <txl/handle_error.h>

#include <optional>

namespace txl
{
    inline auto sendfile(file_base & src, file_base & dst, size_t num_bytes, std::optional<off_t> offset = std::nullopt) -> result<size_t>
    {
        auto res = ::sendfile(dst.fd(), src.fd(), optional_to_ptr(offset), num_bytes);
        return handle_system_error(res, static_cast<size_t>(res));
    }

    struct sendfile_file_base : file_base
    {
        auto copy_from(file_base & src, size_t num_bytes, std::optional<off_t> offset = std::nullopt) -> result<size_t>
        {
            return sendfile(src, *this, num_bytes, offset);
        }
        
        auto copy_to(file_base & dst, size_t num_bytes, std::optional<off_t> offset = std::nullopt) -> result<size_t>
        {
            return sendfile(*this, dst, num_bytes, offset);
        }
    };
}
