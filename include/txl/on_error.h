#pragma once

#include <functional>
#include <sstream>
#include <stdexcept>

namespace txl::on_error
{
    template<class Error>
    using callback = std::function<void(Error)>;

    template<class Error>
    struct capture_value final
    {
        Error & err_;

        auto operator()(Error err)
        {
            err_ = err;
        }
    };

    template<class Error>
    inline auto capture(Error & err) -> capture_value<Error>
    {
        return {err};
    }

    struct ignore final
    {
        template<class Error>
        void operator()(Error)
        {
            // Do nothing
        }
    };
    
    struct throw_on_error final
    {
        template<class Error>
        auto operator()(Error err)
        {
            std::ostringstream ss{};
            ss << err;

            throw std::runtime_error{ss.str()};
        }
    };
}
