#pragma once

#include <filesystem>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <array>

namespace txl
{
    using byte_vector = std::vector<uint8_t>;
    using byte_deque = std::deque<std::byte>;

    template<size_t Size>
    using byte_array = std::array<uint8_t, Size>;
}
