#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdint>

namespace txl
{
    /**
     *
     */
    struct any_addr_t {};
    /**
     *
     */
    static constexpr any_addr_t any_addr{};
    
    /**
     *
     */
    struct none_addr_t {};
    /**
     *
     */
    static constexpr none_addr_t none_addr{};

    /**
     *
     */
    // TODO: use ip_address for sin_addr portion
    class endpoint final
    {
    private:
        sockaddr_in M_addr_{};
    public:
        /**
         *
         */
        endpoint()
        {
            M_addr_.sin_family = AF_INET;
        }

        /**
         *
         */
        endpoint(none_addr_t)
            : endpoint()
        {
            M_addr_.sin_addr.s_addr = INADDR_NONE;
        }

        /**
         *
         */
        endpoint(any_addr_t, uint16_t port)
            : endpoint()
        {
            M_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
            M_addr_.sin_port = htons(port);
        }

        /**
         *
         */
        // TODO: use ip_address as arg
        endpoint(char const * ip, uint16_t port)
            : endpoint()
        {
            M_addr_.sin_addr.s_addr = inet_addr(ip);
            M_addr_.sin_port = htons(port);
        }

        /**
         *
         */
        sockaddr_in const * sockaddr() const { return &M_addr_; }

        /**
         *
         */
        bool operator==(endpoint const & ep) const
        {
            return (M_addr_.sin_addr.s_addr == ep.M_addr_.sin_addr.s_addr &&
                    M_addr_.sin_port == ep.M_addr_.sin_port)
                || (M_addr_.sin_addr.s_addr == INADDR_NONE &&
                    ep.M_addr_.sin_addr.s_addr == INADDR_NONE);
        }

        /**
         *
         */
        bool operator!=(endpoint const & ep) const
        {
            return !(*this == ep);
        }

        /**
         *
         */
        static const endpoint none;
    };

    /**
     *
     */
    const endpoint endpoint::none(none_addr);
}
