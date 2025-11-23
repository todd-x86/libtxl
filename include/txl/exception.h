#pragma once

#include <stdexcept>
#include <string>

namespace txl
{
    class exception : std::exception
    {
    private:
        std::string msg_;
    public:
        exception()
            : exception{"exception thrown"}
        {}

        exception(exception const &) = default;
        exception(exception &&) = default;

        exception(std::string && msg)
            : msg_{std::move(msg)}
        {}

        exception(std::string const & msg)
            : msg_{msg}
        {}

        auto message() const -> std::string const &
        {
            return msg_;
        }

        virtual auto what() const -> char const * throw()
        {
            return msg_.c_str();
        }

        virtual auto get_debug_info(debug_info & di) const -> bool
        {
            return false;
        }
    };

    class system_error : exception
    {
    private:
        std::error_code err_;
    public:
        using exception::exception;

        auto error() const -> std::error_code const & { return err_; }
    };
}
