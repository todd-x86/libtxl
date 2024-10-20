#pragma once

#include <limits>
#include <cstdint>

namespace txl
{
    template<uint8_t, class...>
    class storage_union_base;

    template<uint8_t S, class T>
    class storage_union_base<S, T>
    {
    private:
        uint8_t value_[sizeof(T)];
    public:
        auto invoke_deleter(uint8_t occupied)
        {
            if (occupied == S)
            {
                reinterpret_cast<T *>(&value_[0])->~T();
            }
        }

        operator T() const { return *reinterpret_cast<T const *>(&value_[0]); }

        auto assign(uint8_t & occupied, T && value) -> void
        {
            occupied = S;
            new(&value_[0]) T{std::move(value)};
        }
    };

    template<uint8_t S, class T, class... Args>
    class storage_union_base<S, T, Args...>
    {
    private:
        union
        {
            uint8_t value_[sizeof(T)];
            storage_union_base<S+1, Args...> args_;
        };
    public:
        auto invoke_deleter(uint8_t occupied)
        {
            if (occupied == S)
            {
                reinterpret_cast<T *>(&value_[0])->~T();
            }
            else
            {
                args_.invoke_deleter(occupied);
            }
        }

        operator T() const { return *reinterpret_cast<T const *>(&value_[0]); }

        auto assign(uint8_t & occupied, T && value) -> void
        {
            occupied = S;
            new(&value_[0]) T{std::move(value)};
        }
        
        template<class Value>
        operator Value() const { return static_cast<Value>(args_); }

        template<class Value>
        auto assign(uint8_t & occupied, Value && value) -> void
        {
            args_.assign(occupied, std::move(value));
        }
    };

    template<class... Args>
    class storage_union
    {
    private:
        storage_union_base<0, Args...> base_;
        uint8_t occupied_ = std::numeric_limits<uint8_t>::max();

        auto invoke_deleter() -> void
        {
            if (occupied_ != std::numeric_limits<uint8_t>::max())
            {
                base_.invoke_deleter(occupied_);
            }
        }
    public:
        ~storage_union()
        {
            invoke_deleter();
        }

        template<class T>
        operator T() const { return static_cast<T>(base_); }

        template<class T>
        auto operator=(T value) -> storage_union<Args...> &
        {
            invoke_deleter();
            base_.assign(occupied_, std::move(value));
            return *this;
        }
    };
}
