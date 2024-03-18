#pragma once

namespace txl
{
    template<class Value, class... Values>
    inline bool is_one_of(Value value, Values ... values)
    {
        auto get_first = [](auto value, auto ...) { return value; };
        return ((value == get_first(values)) || ...);
    }
}
