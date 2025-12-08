#pragma once

#include <algorithm>
#include <functional>
#include <vector>
#include <optional>

namespace txl
{
    template<class Value, class Alloc = std::allocator<Value>>
    struct vector : std::vector<Value, Alloc>
    {
        using const_iterator = typename std::vector<Value, Alloc>::const_iterator;
        using iterator = typename std::vector<Value, Alloc>::iterator;
        using value_type = typename std::vector<Value, Alloc>::value_type;

        using std::vector<Value, Alloc>::vector;

        auto erase_at(size_t index) -> void
        {
            this->erase(this->begin() + index);
        }

        auto contains(Value const & value) const -> bool
        {
            return find(value) != this->end();
        }
            
        auto find(Value const & value) const -> const_iterator
        {
            return std::find(this->begin(), this->end(), value);
        }

        auto sort() -> void
        {
            sort(std::less<Value>{});
        }

        template<class Comp>
        auto sort(Comp const & cmp) -> void
        {
            std::sort(this->begin(), this->end(), cmp);
        }

        auto index_of(Value const & value) const -> std::optional<size_t>
        {
            auto it = find(value);
            if (it != this->end())
            {
                return std::distance(this->begin(), it);
            }
            return {};
        }
    };
}
