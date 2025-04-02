#pragma once

#include <cstdint>

namespace txl
{
    inline constexpr auto is_power_of_two(uint64_t value) -> bool
    {
        return value != 0 and (value & (value - 1)) == 0;
    }
}
