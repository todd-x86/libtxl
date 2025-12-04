#pragma once

#include <stdexcept>
#include <string>

namespace txl
{
    struct debug_info_data
    {
        virtual auto get_debug_info(debug_info & di) const -> bool
        {
            return false;
        }
    };

    class exception : public std::exception
                    , public debug_info_data
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
    };

    class system_error : public exception
    {
    private:
        std::error_code err_;
    public:
        using exception::exception;

        auto error() const -> std::error_code const & { return err_; }
    };
}
