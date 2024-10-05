#pragma once

namespace txl
{
    // This differs from std::reference_wrapper<T> because it accepts
    // both l-values and r-values.
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
        auto operator*() -> Value & { return *value_; }
        auto operator*() const -> Value const & { return *value_; }
        auto operator->() -> Value * { return value_; }
        auto operator->() const -> Value const * { return value_; }
    };


    template<class T>
    auto make_ref(T & value) -> ref<T>
    {
        return ref<T>{value};
    }
}
