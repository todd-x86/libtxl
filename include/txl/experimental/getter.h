#pragma once

#include <iostream>

struct Foo
{
    void bar(int & x) const { x = 1; }
    void bar(double & x) const { x = 1.234; }

    template<class T>
    T get() const
    {
        T v;
        bar(v);
        return v;
    }
};

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

int main()
{
    my_union<int, double, char> x;
    x.other.value = 1234.45;
    static_assert(sizeof(x) == 8);

    Foo f;
    f.get<int>();
    f.get<double>();
}
