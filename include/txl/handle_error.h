#pragma once

#include <txl/on_error.h>
#include <txl/system_error.h>

namespace txl
{
    inline auto handle_system_error(int res, on_error::callback<system_error> & on_err) -> bool
    {
        if (res == -1)
        {
            on_err(get_system_error());
            return false;
        }
        return true;
    }
}
