#pragma once

#include <system_error>
#include <errno.h>

namespace txl
{
    inline auto get_system_error(int err) -> std::error_code
    {
        return {err, std::system_category()};
    }
    
    inline auto get_system_error() -> std::error_code
    {
        return get_system_error(errno);
    }
}
