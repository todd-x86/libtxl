#pragma once

#include <algorithm>
#include <type_traits>

namespace txl
{
    namespace detail
    {
        template<class T, class = void>
        struct is_map : std::false_type
        {
        };

        template<class T>
        struct is_map<T, std::void_t<typename T::mapped_type>> : std::true_type
        {
        };
    }

    template<class Container, class ValueOrFunc>
    inline auto find_or_emplace(Container & container, typename Container::key_type const & key, ValueOrFunc && value_or_func) -> typename Container::iterator
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

    template<class Container, class ValueOrFunc>
    inline auto emplace_or_update(Container & container, typename Container::key_type const & key, ValueOrFunc && value_or_func) -> typename Container::iterator
    {
        constexpr auto is_value_movable = (std::is_same_v<ValueOrFunc, typename Container::mapped_type> or std::is_convertible_v<ValueOrFunc, typename Container::mapped_type>);

        if (auto it = container.find(key); it != container.end())
        {
            if constexpr (is_value_movable)
            {
                it->second = std::move(value_or_func);
            }
            else
            {
                it->second = value_or_func(key);
            }
            return it;
        }

        if constexpr (is_value_movable)
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

    template<class Container, class Key, class Func>
    inline auto if_found(Container const & container, Key const & key, Func func) -> bool
    {
        if constexpr (detail::is_map<Container>::value)
        {
            if (auto it = container.find(key); it != container.end())
            {
                func(it);
                return true;
            }
        }
        else
        {
            if (auto it = std::find(container.begin(), container.end(), key); it != container.end())
            {
                func(it);
                return true;
            }
        }

        return false;
    }

    template<class Container, class ValueOrFunc>
    inline auto try_emplace(Container & container, typename Container::key_type const & key, ValueOrFunc value_or_func) -> bool
    {
        if (auto it = container.find(key); it != container.end())
        {
            return false;
        }

        if constexpr (std::is_same_v<ValueOrFunc, typename Container::mapped_type> or std::is_convertible_v<ValueOrFunc, typename Container::mapped_type>)
        {
            // Pass as value
            auto [_, emplaced] = container.emplace(key, std::move(value_or_func));
            return emplaced;
        }
        else
        {
            // Invoke as function
            auto value = value_or_func(key);
            auto [_, emplaced] = container.emplace(key, static_cast<typename Container::mapped_type &&>(std::move(value)));
            return emplaced;
        }
    }
}
