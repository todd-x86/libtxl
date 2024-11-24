#pragma once

#include <txl/result.h>
#include <txl/system_error.h>

#include <type_traits>

namespace txl
{
    inline auto handle_system_error(int res) -> result<void>
    {
        if (res == -1)
        {
            return {get_system_error()};
        }
        return {};
    }
    
    template<class T>
    inline auto handle_system_error(int res, T && value) -> result<std::remove_reference_t<T>>
    {
        if (res == -1)
        {
            return {get_system_error()};
        }
        return {std::move(value)};
    }
    
    template<class T>
    inline auto handle_system_error(int res, T const & value) -> result<std::remove_reference_t<T>>
    {
        if (res == -1)
        {
            return {get_system_error()};
        }
        return {value};
    }
}
