#pragma once

#include <functional>
#include <stdexcept>
#include <type_traits>

namespace txl::detail
{
    template<class>
    struct remove_member_ptr {};

    template<class Ret, class T, class... Args>
    struct remove_member_ptr<Ret (T::*)(Args...)>
    {
        using type = Ret(Args...);
    };

    template<class Ret, class T, class... Args>
    struct remove_member_ptr<Ret (T::*)(Args...) const>
    {
        using type = Ret(Args...);
    };

    template<class T>
    using remove_member_ptr_t = typename remove_member_ptr<T>::type;
}

#define FUNCTION_LIKE(method_ptr) std::function<::txl::detail::remove_member_ptr_t<decltype(&method_ptr)>>
#define CLASS_METHOD(method) [this]<class... Args>(Args && ... args) { return method(std::forward<Args>(args)...); }
#define BIND_FEATURE(field, public_name) template<class... Args> auto public_name(Args && ... args) { \
    if constexpr (std::is_void_v<decltype(field(std::forward<Args>(args)...))>) { \
        if (field) { \
            field(std::forward<Args>(args)...); \
        } \
    } else { \
        if (field) { \
            return field(std::forward<Args>(args)...); \
        } \
        throw std::runtime_error{"Feature '" #public_name "' is not defined"}; \
    } \
}
