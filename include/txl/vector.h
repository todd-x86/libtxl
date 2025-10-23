#pragma once

#include <vector>
#include <optional>

namespace txl
{
    template<class Value, class Alloc = std::allocator<Value>>
    struct vector : std::vector<Value, Alloc>
    {
        auto erase_at(size_t index) -> void
        {
            this->erase(this->begin() + index);
        }

        auto contains(Value const & value) const -> bool
        {
            return index_of(value).has_value();
        }

        auto index_of(Value const & value) const -> std::optional<size_t>
        {
            auto it = std::find(this->begin(), this->end(), value);
            if (it != this->end())
            {
                return std::distance(this->begin(), it);
            }
            return {};
        }
    };
}
