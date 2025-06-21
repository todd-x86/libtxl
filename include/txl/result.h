#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <system_error>

namespace txl
{
    template<class Value>
    class result final
    {
    private:
        struct is_value_t final
        {
        };

        struct is_error_t final
        {
        };

        enum flags : uint8_t
        {
            ASSIGNED = 1 << 0,
            IS_ERROR = 1 << 1,
        };

        union
        {
            Value value_;
            std::error_code error_;
        };
        uint8_t flags_ = 0;

        auto move_from(result && v) -> void
        {
            std::swap(flags_, v.flags_);

            if (flags_ & ASSIGNED)
            {
                if (flags_ & IS_ERROR)
                {
                    error_ = std::move(v.error_);
                }
                else
                {
                    value_ = std::move(v.value_);
                }
            }
        }
    public:
        result()
        {
        }

        result(Value && value)
            : value_(std::move(value))
            , flags_(ASSIGNED)
        {
        }

        result(std::error_code const & err)
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
                error_.~error_code();
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
        auto value() const -> Value const & { return value_; }
        auto error() const -> std::error_code const & { return error_; }

        auto or_throw() -> Value &&
        {
            if (not is_error())
            {
                return release();
            }
            throw std::system_error{error()};
        }

        auto or_value(Value && v) -> Value &&
        {
            if (not is_error())
            {
                return release();
            }
            return std::move(v);
        }

        auto as_optional() -> std::optional<Value>
        {
            if (not is_error())
            {
                return {release()};
            }
            return {std::nullopt};
        }
        
        auto release() -> Value && { return std::move(value_); }

        auto then(std::function<result<Value>()> cont) -> result<Value> &
        {
            // Only runs if result is NOT an error
            if (not is_error())
            {
                *this = std::move(cont());
            }
            return *this;
        }
    };
    
    template<>
    class result<void> final
    {
    private:
        std::error_code error_{};
    public:
        result() = default;

        result(std::error_code const & err)
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
        auto error() const -> std::error_code const & { return error_; }

        auto or_throw() -> void
        {
            if (is_error())
            {
                throw std::system_error{error()};
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
}
