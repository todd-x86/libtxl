#pragma once

#include <sys/socket.h>

namespace txl::socket_option
{
    template<class Value>
    struct option
    {
        int const level;
        int const opt_name;

        constexpr option(int _level, int _opt_name)
            : level{_level}
            , opt_name{_opt_name}
        {
        }
    };

    static constexpr const option<::linger> linger{SOL_SOCKET, SO_LINGER};
}
