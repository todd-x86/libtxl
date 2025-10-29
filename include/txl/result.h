#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <system_error>

namespace txl
{
    struct system_error_context
    {
        using error_type = std::error_code;
        using exception_type = std::system_error;
    };

    struct empty_result_error : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    namespace detail
    {
        template<class Value>
        struct result_value_wrapper final
        {
            Value const & value;
            
            result_value_wrapper(Value const & value)
                : value(value)
            {
            }
        };

        template<class Error>
        struct result_error_wrapper final
        {
            Error const & error;
            
            result_error_wrapper(Error const & error)
                : error(error)
            {
            }
        };
    }

    template<class Value>
    inline auto as_result(Value const & val) -> detail::result_value_wrapper<Value>
    {
        return {val};
    }
    
    template<class Error>
    inline auto as_error(Error const & err) -> detail::result_error_wrapper<Error>
    {
        return {err};
    }

    template<class Value, class ErrorContext = system_error_context>
    class result final
    {
    private:
        using error_type = typename ErrorContext::error_type;
        
        enum flags : uint8_t
        {
            ASSIGNED = 1 << 0,
            IS_ERROR = 1 << 1,
        };

        union
        {
            Value value_;
            error_type error_;
        };
        uint8_t flags_ = 0;

        auto move_from(result && v) -> void
        {
            auto has_previous_value = flags_ & ASSIGNED;
            std::swap(flags_, v.flags_);

            if (flags_ & ASSIGNED)
            {
                if (flags_ & IS_ERROR)
                {
                    error_ = std::move(v.error_);
                }
                else
                {
                    if (has_previous_value)
                    {
                        value_.~Value();
                    }
                    new(&value_) Value(std::move(v.value_));
                }
            }
        }
    public:
        using value_type = Value;
        using error_context_type = ErrorContext;

        result()
        {
        }

        template<class V>
        result(detail::result_value_wrapper<V> && val)
            : value_(std::move(const_cast<V &>(val.value)))
            , flags_(ASSIGNED)
        {
        }

        template<class E>
        result(detail::result_error_wrapper<E> && error)
            : error_(error.error)
            , flags_(ASSIGNED | IS_ERROR)
        {
        }

        result(Value && value)
            : value_(std::move(value))
            , flags_(ASSIGNED)
        {
        }

        result(error_type const & err)
            : error_(err)
            , flags_(ASSIGNED | IS_ERROR)
        {
        }

        result(result && v)
        {
            move_from(std::move(v));
        }
        
        result(result const & v)
            : result()
        {
            flags_ = v.flags_;
            if (flags_ & ASSIGNED)
            {
                if (flags_ & IS_ERROR)
                {
                    error_ = v.error_;
                }
                else
                {
                    value_ = v.value_;
                }
            }
        }

        ~result()
        {
            if ((flags_ & ASSIGNED) == 0)
            {
                return;
            }

            if (flags_ & IS_ERROR)
            {
                error_.~error_type();
            }
            else
            {
                value_.~Value();
            }
        }
        
        auto operator=(result const &) -> result & = default;
        auto operator=(result && v) -> result &
        {
            if (&v != this)
            {
                move_from(std::move(v));
            }
            return *this;
        }

        auto operator->() -> Value * { return &value_; }
        auto operator->() const -> Value const * { return &value_; }
        auto operator*() -> Value & { return value_; }
        auto operator*() const -> Value const & { return value_; }

        operator bool() const { return not is_error(); } 

        auto is_assigned() const -> bool { return flags_ & ASSIGNED; }
        auto empty() const -> bool { return not is_assigned(); }
        auto is_error() const -> bool { return flags_ & IS_ERROR; }
        auto is_error(int err_code) const -> bool { return is_error() and error().value() == err_code; }
        auto value() const -> Value const & { return value_; }
        auto error() const -> error_type const & { return error_; }

        auto assign_using() -> Value &
        {
            flags_ |= ASSIGNED;
            return value_;
        }
        
        template<class ValueOrFunc>
        auto ignore(error_type const & err, ValueOrFunc && value_or_func) -> result
        {
            if (is_error() and error() == err)
            {
                if constexpr (std::is_same_v<ValueOrFunc, Value> or std::is_convertible_v<ValueOrFunc, Value>)
                {
                    // Value
                    return {std::move(value_or_func)};
                }
                else
                {
                    // Function
                    return {value_or_func()};
                }
            }
            return *this;
        }

        auto or_throw() -> Value &&
        {
            if (not is_assigned())
            {
                throw empty_result_error{"result is empty"};
            }
            if (not is_error())
            {
                return release();
            }
            throw typename ErrorContext::exception_type{error()};
        }

        auto or_value(Value && v) -> Value &&
        {
            if (is_assigned() and not is_error())
            {
                return release();
            }
            return std::move(v);
        }

        auto as_optional() -> std::optional<Value>
        {
            if (is_assigned() and not is_error())
            {
                return {release()};
            }
            return {std::nullopt};
        }
        
        auto release() -> Value &&
        {
            flags_ &= ~ASSIGNED;
            return std::move(value_);
        }

        auto then(std::function<result<Value>()> cont) -> result<Value> &
        {
            // Only runs if result is NOT an error
            if (is_assigned() and not is_error())
            {
                *this = std::move(cont());
            }
            return *this;
        }
    };
    
    template<class ErrorContext>
    class result<void, ErrorContext> final
    {
    private:
        using error_type = typename ErrorContext::error_type;

        error_type error_{};
    public:
        result() = default;

        result(error_type const & err)
            : error_(err)
        {
        }

        result(result && v)
            : error_(std::move(v.error_))
        {
        }
        result(result const &) = default;

        auto operator=(result const &) -> result & = default;
        auto operator=(result && v) -> result &
        {
            if (&v != this)
            {
                error_ = std::move(v.error_);
            }
            return *this;
        }

        operator bool() const { return not is_error(); } 

        auto is_assigned() const -> bool { return static_cast<bool>(error_); }
        auto empty() const -> bool { return not is_assigned(); }
        auto is_error() const -> bool { return static_cast<bool>(error_); }
        auto error() const -> error_type const & { return error_; }

        auto ignore(error_type const & err) -> result
        {
            if (is_error() and error() == err)
            {
                return {};
            }
            return *this;
        }

        auto or_throw() -> void
        {
            if (is_error())
            {
                throw typename ErrorContext::exception_type{error()};
            }
        }

        auto then(std::function<result()> cont) -> result &
        {
            // Only runs if result is NOT an error
            if (not is_error())
            {
                *this = cont();
            }
            return *this;
        }
    };

    template<class T, class E>
    inline auto operator<<(std::ostream & os, result<T, E> const & res) -> std::ostream &
    {
        if (not res.is_assigned())
        {
            os << "{empty}";
            return os;
        }
        if (res.is_error())
        {
            os << "{error=" << res.error() << "}";
            return os;
        }
        os << "{result=" << res.value() << "}";
        return os;
    }
}
