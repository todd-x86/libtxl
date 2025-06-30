#pragma once

#include <txl/result.h>

#include <system_error>
#include <errno.h>

namespace txl
{
    template<class T>
    inline auto is_unavailable(result<T> const & res) -> bool
    {
        return res.is_error(EAGAIN);
    }

    template<class T>
    inline auto is_busy(result<T> const & res) -> bool
    {
        return res.is_error(EBUSY);
    }
}
