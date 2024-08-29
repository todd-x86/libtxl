#pragma once

#include <txl/result.h>

#include <ostream>
#include <string_view>
#include <errno.h>

namespace txl
{
    class system_error
    {
    private:
        int code_ = 0;
    public:
        system_error() = default;
        system_error(int code)
            : code_(code)
        {
        }

        auto is_error() const -> bool { return code_ != 0; }
        auto code() const -> int { return code_; }
        auto message() const -> std::string_view { return {::strerror(code_)}; }
    };

    auto operator<<(std::ostream & os, system_error err) -> std::ostream &
    {
        os << err.message();
        return os;
    }

    inline auto get_system_error() -> system_error
    {
        return system_error{errno};
    }

    template<class T>
    using system_result = result<system_error, T>;
}
