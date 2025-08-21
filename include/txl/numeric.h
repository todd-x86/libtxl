#pragma once

#include <cstdlib>
#include <cstdint>

namespace txl
{
    inline auto align(int64_t value, int64_t alignment) -> int64_t
    {
        auto val = std::div(value, alignment);
        return (val.quot + (val.rem != 0)) * alignment;
    }
}
