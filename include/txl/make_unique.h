#pragma once

#if __cplusplus <= 201103L

#include <memory>

namespace std
{
    template<class T, class... _Args>
    unique_ptr<T> make_unique(_Args && ... args)
    {
        return unique_ptr<T>(new T(std::forward<_Args>(args)...));
    }
}

#endif
