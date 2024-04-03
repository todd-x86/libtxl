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

    operator Value&() { return *value_; }
    Value * operator->() { return value_; }
    Value & operator*() { return *value_; }
};

}
