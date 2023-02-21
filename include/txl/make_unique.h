#pragma once

#include <memory>

#if __cplusplus <= 201103L

namespace std
{
    template<class T, class... Args>
    unique_ptr<T> make_unique(Args && ... args)
    {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

#endif
