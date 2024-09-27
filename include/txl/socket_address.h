#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdint>

namespace txl
{
    class socket_address final
    {
        friend class socket;
    private:
        ::sockaddr_in addr_{};
    public:
        socket_address()
        {
            addr_.sin_family = AF_INET;
            addr_.sin_addr.s_addr = INADDR_NONE;
        }

        socket_address(uint16_t port)
            : socket_address()
        {
            addr_.sin_addr.s_addr = htonl(INADDR_ANY);
            addr_.sin_port = htons(port);
        }

        socket_address(char const * ip, uint16_t port)
            : socket_address()
        {
            addr_.sin_addr.s_addr = inet_addr(ip);
            addr_.sin_port = htons(port);
        }

        auto sockaddr() const -> ::sockaddr_in const * { return &addr_; }
        static constexpr auto size() -> size_t { return sizeof(addr_); }

        auto operator==(socket_address const & sa) const -> bool
        {
            return (addr_.sin_addr.s_addr == sa.addr_.sin_addr.s_addr and
                    addr_.sin_port == sa.addr_.sin_port)
                or (addr_.sin_addr.s_addr == INADDR_NONE and
                    sa.addr_.sin_addr.s_addr == INADDR_NONE);
        }

        auto operator!=(socket_address const & sa) const -> bool
        {
            return !(*this == sa);
        }
    };
}
