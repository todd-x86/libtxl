#pragma once

#include <utility>

//#define OPTIONALLY(x, call) x.with([&](auto & x) { x.call; })
#define OPTIONALLY(x, call) OPT_CALL(x, call)
#define OPT_CALL(x, call) do { if (x) { x->call; } } while (false)
#define OPT_RETURN(x, call) do { if (x) { return x->call; } } while (false)

namespace txl
{
    template<class T>
    class nullable final
    {
    private:
        T * value_ = nullptr;
    public:
        nullable() = default;

        nullable(T & value)
            : value_{&value}
        {
        }
        
        nullable(T && value)
            : value_{&value}
        {
        }

        operator bool() const
        {
            return has_value();
        }

        auto operator*() -> T &
        {
            return *value_;
        }

        auto operator->() -> T *
        {
            return value_;
        }

        auto has_value() const -> bool
        {
            return value_ != nullptr;
        }

        auto is_null() const -> bool
        {
            return not has_value();
        }

        template<class Func>
        auto with(Func && func)
        {
            if (value_)
            {
                func(*value_);
            }
        }
    };
}
