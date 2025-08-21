#pragma once

#include <txl/iterator_view.h>
#include <txl/result.h>
#include <txl/result_error.h>
#include <txl/type_info.h>

#include <limits>
#include <type_traits>
#include <string_view>

namespace txl
{
    enum class conversion_error : int
    {
        none = 0,
        invalid_input,
        unknown,
    };

    struct conversion_error_context
    {
        using error_type = conversion_error;
        using exception_type = result_error<conversion_error>;
    };

    template<class T>
    using convert_result = result<T, conversion_error_context>;
    
    struct octet_string_view : std::string_view
    {
        using std::string_view::string_view;
    };
    
    //template<class Dst, class Src>
    //inline auto convert_to(Src const & src) -> convert_result<Dst>;

    template<class Int, class = std::enable_if_t<std::numeric_limits<Int>::is_integer>>
    inline auto convert_to(octet_string_view const & src) -> convert_result<Int>
    {
        Int dec = 0;
        for (auto ch : src)
        {
            dec <<= 3;
            if (ch < '0' or ch > '9')
            {
                return {conversion_error::invalid_input};
            }
            dec += (ch-'0');
        }
        return dec;
    }
}
