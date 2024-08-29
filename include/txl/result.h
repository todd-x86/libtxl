#pragma once

#include <txl/on_error.h>

#include <optional>
#include <stdexcept>
#include <sstream>

namespace txl
{
    template<class Error, class Value>
    class result final
    {
    private:
        std::optional<Value> value_;
        std::optional<Error> err_;
    public:
        result(std::optional<Value> && value_opt, std::optional<Error> && err_opt)
            : value_(std::move(value_opt))
            , err_(std::move(err_opt))
        {
        }
        
        result(Value value)
            : result(std::move(value), std::nullopt)
        {
        }

        result(Value value, Error err)
            : result(std::make_optional<Value>(std::move(value)), std::make_optional<Error>(std::move(err)))
        {
        }

        template<class E>
        auto with_error(E err) -> result<E, Value>
        {
            return {std::move(value_), std::forward<E>(err)};
        }

        template<class V>
        auto with_value(V value) -> result<Error, V>
        {
            return {std::make_optional<V>(std::move(value)), std::move(err_)};
        }

        auto has_error() const -> bool { return err_.has_value(); }
        auto has_value() const -> bool { return value_.has_value(); }
        auto ok() const -> bool { return not has_error(); }

        auto error() const -> Error const & { return *err_; }
        auto value() const -> Value const & { return *value_; }
        auto release_value() -> Value && { return std::move(value_.value()); }

        auto operator*() const -> Value const & { return value(); }
        auto operator->() const -> Value const * { return &(value()); }
        
        auto or_throw() -> Value &&
        {
            if (has_error())
            {
                on_error::throw_on_error{}(error());
            }
            return std::move(release_value());
        }
    };
}
