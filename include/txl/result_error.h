#pragma once

#include <stdexcept>
#include <string>

namespace txl
{
    template<class Error>
    class result_error : public std::runtime_error
    {
    private:
        Error err_;
    public:
        result_error(Error const & err)
            : result_error{"result yielded an error", err}
        {
        }

        result_error(std::string const & msg, Error const & err)
            : std::runtime_error::runtime_error{msg}
            , err_{err}
        {
        }

        auto error() const -> Error const & { return err_; }
    };
}
