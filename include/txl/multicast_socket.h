#pragma once

#include <txl/socket.h>
#include <txl/socket_option.h>
#include <txl/socket_address.h>
#include <txl/result.h>

namespace txl
{
    struct multicast_socket : socket
    {
        multicast_socket() = default;

        auto open() -> result<void>
        {
            return socket::open(socket::internet, socket::datagram, 0);
        }

        auto join_multicast(socket_address const & sa) -> result<void>
        {
            ::ip_mreq req{};
            req.imr_multiaddr.s_addr = sa.sockaddr()->sin_addr.s_addr;
            req.imr_interface.s_addr = htonl(INADDR_ANY);
            return set_option(::txl::socket_option::join_multicast, req);
        }
        
        auto leave_multicast(socket_address const & sa) -> result<void>
        {
            ::ip_mreq req{};
            req.imr_multiaddr.s_addr = sa.sockaddr()->sin_addr.s_addr;
            req.imr_interface.s_addr = htonl(INADDR_ANY);
            return set_option(::txl::socket_option::leave_multicast, req);
        }
    };
}
