#pragma once

#include <functional>

namespace txl
{
    template<class T>
    struct ref : std::reference_wrapper<T>
    {
        auto operator*() -> T & { return this->get(); }
        auto operator*() const -> T const & { return this->get(); }
    };

    template<class T>
    auto make_ref(T & value) -> ref<T>
    {
        return ref<T>{value};
    }
}
