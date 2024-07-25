#pragma once

#include <type_traits>

namespace txl
{

template<class Container, class Key, class ValueOrFunc>
inline auto find_or_emplace(Container & container, Key const & key, ValueOrFunc value_or_func) -> typename Container::iterator
{
    if (auto it = container.find(key); it != container.end())
    {
        return it;
    }

    if constexpr (std::is_same_v<ValueOrFunc, typename Container::mapped_type> or std::is_convertible_v<ValueOrFunc, typename Container::mapped_type>)
    {
        // Pass as value
        auto [it, _] = container.emplace(key, std::move(value_or_func));
        return it;
    }
    else
    {
        // Invoke as function
        auto value = value_or_func(key);
        auto [it, _] = container.emplace(key, static_cast<typename Container::mapped_type &&>(std::move(value)));
        return it;
    }
}

}
