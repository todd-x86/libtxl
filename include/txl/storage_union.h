#pragma once

// Alternative for std::variant with a more union-like interface (no safeties)

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
        template<class C>
        auto contains(uint8_t occupied) const -> bool
        {
            if constexpr (std::is_same_v<T, C>)
            {
                return (S == occupied);
            }
            return false;
        }

        auto invoke_deleter(uint8_t occupied)
        {
            if (occupied == S)
            {
                reinterpret_cast<T *>(&value_[0])->~T();
            }
        }

        template<class R>
        auto release() -> R &&
        {
            if constexpr (std::is_same_v<R, T>)
            {
                return std::move(*reinterpret_cast<R *>(&value_[0]));
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
        auto data() const -> void const * { return reinterpret_cast<void const *>(&value_[0]); }
        auto data() -> void * { return reinterpret_cast<void *>(&value_[0]); }

        template<class C>
        auto contains(uint8_t occupied) const -> bool
        {
            if constexpr (std::is_same_v<T, C>)
            {
                return (S == occupied);
            }
            return args_.template contains<C>(occupied);
        }

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
        
        template<class R>
        auto release() -> R &&
        {
            if constexpr (std::is_same_v<R, T>)
            {
                return std::move(*reinterpret_cast<R *>(&value_[0]));
            }
            return std::move(args_.template release<R>());
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
        uint8_t padding_[sizeof(void *)-sizeof(uint8_t)];

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
        auto ref() const -> T const & { return *reinterpret_cast<T const *>(base_.data()); }
        
        template<class T>
        auto ref() -> T & { return *reinterpret_cast<T *>(base_.data()); }

        auto empty() const -> bool { return occupied_ == std::numeric_limits<uint8_t>::max(); }

        auto reset() -> void { occupied_ = std::numeric_limits<uint8_t>::max(); }

        template<class T>
        auto has() const -> bool { return base_.template contains<T>(occupied_); }

        template<class T>
        auto get() const -> T { return static_cast<T>(base_); }

        template<class T>
        auto release() -> T
        {
            auto res = std::move(base_.template release<T>());
            reset();
            return res;
        }

        template<class T>
        auto set(T && value) -> void
        {
            invoke_deleter();
            base_.assign(occupied_, std::move(value));
        }

        template<class T>
        operator T() const { return static_cast<T>(base_); }

        template<class T>
        auto operator=(T value) -> storage_union<Args...> &
        {
            set(std::move(value));
            return *this;
        }
    };
}
