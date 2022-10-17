#pragma once

#include <iostream>

template<class T1, class... _Args>
union my_union;

template<class T1>
union my_union<T1>
{
    T1 value;
};

template<class T1, class... _Args>
union my_union
{
    T1 value;
    my_union<_Args...> other;
};

/*template<class T, class... _Args>
T & get(my_union<_Args...> & u)
{
    if constexpr (u.type == t) {

    }
}*/

int main()
{
    my_union<int, double, char> x;
    x.other.value = 1234.45;
    static_assert(sizeof(x) == 8);
}
