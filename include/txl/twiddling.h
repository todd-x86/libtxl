#pragma once

#include <cstdint>

namespace llio
{
    inline constexpr bool is_power_of_two(uint64_t value)
    {
        return value != 0 && (value & (value - 1)) == 0;
    }
}
