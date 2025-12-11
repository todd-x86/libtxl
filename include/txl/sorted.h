#pragma once

#include <txl/patterns.h>

#include <algorithm>
#include <functional>

namespace txl
{
    namespace detail
    {
        template<class, class = void>
        struct has_sort_method : std::false_type {};

        template<class T>
        struct has_sort_method<T, std::void_t<decltype(std::declval<T>().sort())>> : std::true_type {};
    }

    template<class T, template<class> class Less = std::less>
    struct sorted : public T
    {
        using value_type = typename T::value_type;
        using T::T;

        sorted(std::initializer_list<value_type> vals)
            : T{std::move(vals)}
        {
            this->sort();
        }

        template<class... Args>
        auto emplace_sorted(Args && ... args)
        {
            value_type val(std::forward<Args>(args)...);
            auto it = upper_bound(*this, val, Less<value_type>{});
            return this->emplace(it, std::move(val));
        }
        
        auto insert_sorted(value_type const & val)
        {
            auto it = upper_bound(*this, val, Less<value_type>{});
            return this->insert(it, val);
        }

        auto sort()
        {
            if constexpr (detail::has_sort_method<T>::value)
            {
                T::sort();
            }
            else
            {
                std::sort(this->begin(), this->end(), Less<value_type>{});
            }
        }
    };
}
