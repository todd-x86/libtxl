#pragma once

namespace txl {

template<class Value>
class ref final
{
private:
    Value * value_;
public:
    ref(Value & value)
        : value_(&value)
    {
    }
    ref(Value && value)
        : value_(&value)
    {
        // Value should last long enough here
    }

    operator Value&() { return *value_; }
    Value * operator->() { return value_; }
    Value & operator*() { return *value_; }
};

template<class T>
inline auto make_ref(T & value) -> ref<T>
{
    return ref<T>{value};
}

}
