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
    static constexpr const option<::timeval> recv_timeout{SOL_SOCKET, SO_RCVTIMEO};
    static constexpr const option<::timeval> send_timeout{SOL_SOCKET, SO_SNDTIMEO};
    static constexpr const option<::ip_mreq> join_multicast{IPPROTO_IP, IP_ADD_MEMBERSHIP};
    static constexpr const option<::ip_mreq> leave_multicast{IPPROTO_IP, IP_DROP_MEMBERSHIP};
    static constexpr const option<int> reuse_address{SOL_SOCKET, SO_REUSEADDR};
    static constexpr const option<int> reuse_port{SOL_SOCKET, SO_REUSEPORT};
}
