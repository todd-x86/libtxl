#pragma once

namespace txl
{
    template<class Value>
    class storage
    {
    private:
        alignas(Value) std::byte val_[sizeof(Value)];
        auto val() -> Value & { return *reinterpret_cast<Value *>(&val_[0]); }
    public:
        storage() = default;
        virtual ~storage()
        {
            val().~Value();
        }

        auto operator*() -> Value & { return val(); }
        auto operator*() -> Value const & { return *reinterpret_cast<Value const *>(&val_[0]); }
        auto operator->() -> Value * { return reinterpret_cast<Value *>(&val_[0]); }
        auto operator->() -> Value const * { return reinterpret_cast<Value const *>(&val_[0]); }
    };
}
