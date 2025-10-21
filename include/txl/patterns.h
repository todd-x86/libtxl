#pragma once

#include <algorithm>
#include <type_traits>
#include <map>
#include <optional>
#include <unordered_map>
#include <functional>

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

    template<class T>
    struct virtual_iterator
    {
    private:
        T const * last_ = nullptr;
    public:
        virtual auto get_ref() const -> T const & = 0;
        virtual auto next() -> T const * = 0;

        auto get_ptr() -> T const *
        {
            if (last_ == nullptr)
            {
                last_ = &get_ref();
            }
            return last_;
        };

        auto get_ptr() const -> T const *
        {
            if (last_ == nullptr)
            {
                return &get_ref();
            }
            return last_;
        }

        auto operator->() const -> T const *
        {
            return get_ptr();
        }

        auto operator*() const -> T const &
        {
            return *get_ptr();
        }

        auto operator==(virtual_iterator const & it) const -> bool
        {
            return get_ptr() == it.get_ptr();
        }

        auto operator!=(virtual_iterator const & it) const -> bool
        {
            return !(*this == it);
        }

        auto operator++() -> virtual_iterator &
        {
            last_ = next();
            return *this;
        }
    };

    template<class Iter, class Value = typename std::iterator_traits<Iter>::value_type>
    struct virtual_iterator_wrapper final : virtual_iterator<Value>
    {
    private:
        Iter it_;
    public:
        virtual_iterator_wrapper(Iter it)
            : it_{it}
        {
        }

        auto next() -> Value const * override
        {
            ++it_;
            return &(*it_);
        }

        auto get_ref() const -> Value const & override
        {
            return *it_;
        }
    };

    template<class T>
    class virtual_iterator_view final
    {
    private:
        virtual_iterator<T> & begin_;
        virtual_iterator<T> & end_;
    public:
        struct const_iterator final
        {
            virtual_iterator<T> * it_;

            const_iterator(virtual_iterator<T> & it)
                : it_{&it}
            {
            }

            auto operator->() const -> T const *
            {
                return &(**it_);
            }

            auto operator*() const -> T const &
            {
                return **it_;
            }

            auto operator==(const_iterator const & it) const -> bool
            {
                return *it.it_ == *it_;
            }

            auto operator!=(const_iterator const & it) const -> bool
            {
                return !(*this == it);
            }

            auto operator++() -> const_iterator &
            {
                ++(*it_);
                return *this;
            }
        };

        virtual_iterator_view(virtual_iterator<T> & begin, virtual_iterator<T> & end)
            : begin_(begin)
            , end_(end)
        {
        }

        auto begin() const -> const_iterator { return begin_; }
        auto end() const -> const_iterator { return end_; }
    };

    /**
     * Lightweight interface on iterating over a generic collection.
     */
    template<class Value>
    struct foreach_view
    {
        virtual auto foreach(std::function<void(Value const &)> on_element) const -> void = 0;
    };

    template<class Container, class Value = typename Container::value_type>
    class container_foreach_view final : public foreach_view<Value>
    {
    private:
        Container const & container_;
    public:
        container_foreach_view(Container const & container)
            : container_{container}
        {
        }

        auto foreach(std::function<void(Value const &)> on_element) const -> void override
        {
            for (auto const & el : container_)
            {
                on_element(el);
            }
        }
    };

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

    template<class ContainerOrView, class Key>
    inline auto lower_bound(ContainerOrView const & container, Key const & key)
    {
        return std::lower_bound(container.begin(), container.end(), key);
    }

    template<class ContainerOrView, class Key, class CompareFunc>
    inline auto lower_bound(ContainerOrView const & container, Key const & key, CompareFunc comp)
    {
        return std::lower_bound(container.begin(), container.end(), key, comp);
    }

    template<class Value>
    inline auto optional_to_ptr(std::optional<Value> const & opt) -> Value const *
    {
        if (opt)
        {
            return &opt.value();
        }

        return nullptr;
    }

    template<class Value>
    inline auto optional_to_ptr(std::optional<Value> & opt) -> Value *
    {
        if (opt)
        {
            return &opt.value();
        }

        return nullptr;
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

    template<class Container, class Value = typename Container::value_type>
    inline auto to_vector(Container const & container) -> std::vector<Value>
    {
        std::vector<Value> res{};
        for (auto const & item : container)
        {
            res.push_back(item);
        }
        return res;
    }

    template<class Container, class KeyType = std::remove_cv_t<typename Container::value_type::first_type>, class MappedType = typename Container::value_type::second_type>
    inline auto to_map(Container const & container) -> std::map<KeyType, MappedType>
    {
        std::map<KeyType, MappedType> res{};
        for (auto const & [key, value] : container)
        {
            res.emplace(key, value);
        }
        return res;
    }

    template<class Container, class KeyType = std::remove_cv_t<typename Container::value_type::first_type>, class MappedType = typename Container::value_type::second_type>
    inline auto to_unordered_map(Container const & container) -> std::unordered_map<KeyType, MappedType>
    {
        std::unordered_map<KeyType, MappedType> res{};
        for (auto const & [key, value] : container)
        {
            res.emplace(key, value);
        }
        return res;
    }
    
    template<class ContainerOrView, class Key>
    inline auto upper_bound(ContainerOrView const & container, Key const & key)
    {
        return std::upper_bound(container.begin(), container.end(), key);
    }

    template<class ContainerOrView, class Key, class CompareFunc>
    inline auto upper_bound(ContainerOrView const & container, Key const & key, CompareFunc comp)
    {
        return std::upper_bound(container.begin(), container.end(), key, comp);
    }
}
