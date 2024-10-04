#pragma once

#include <vector>
#include <cstdint>
#include <cstdlib>
#include <array>

namespace txl
{
    using byte_vector = std::vector<uint8_t>;

    template<size_t Size>
    using byte_array = std::array<uint8_t, Size>;
}
