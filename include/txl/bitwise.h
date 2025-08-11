#pragma once

#include <cstdint>

namespace txl
{
    /**
     * Simple bitwise operator to determine if a value is a power of 2.
     *
     * \param value value to inspect
     * \return true if value is not 0 and is divisible by 2
     */
    inline constexpr auto is_power_of_two(uint64_t value) -> bool
    {
        return value != 0 and (value & (value - 1)) == 0;
    }
}
