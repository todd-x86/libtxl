#pragma once

#include <txl/socket_address.h>
#include <txl/socket.h>
#include <txl/result.h>
#include <txl/read_string.h>
#include <txl/size_policy.h>
#include <iostream>

namespace txl::http
{
    class test_server
    {
    private:
        ::txl::socket listener_{};
    public:
        auto open(::txl::socket_address addr) -> ::txl::result<void>
        {
            if (auto res = listener_.open(::txl::socket::internet, ::txl::socket::stream, 0); not res)
            {
                return res;
            }
            return listener_.bind(addr);
        }

        auto run() -> ::txl::result<void>
        {
            open(::txl::socket_address{"0.0.0.0", 8012}).or_throw();
            listener_.set_option(::txl::socket_option::linger, ::linger{1,0}).or_throw();
            do
            {
                auto res = listener_.listen(0);
                if (not res)
                {
                    return res;
                }

                auto client = listener_.accept().or_throw();
                auto req = ::txl::read_string(client, ::txl::one_of{::txl::until{"\n\n"}, ::txl::at_most{1024}}).or_throw();
                auto s = std::ostringstream{};
                s << "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<h1>Hello</h1>";
                s << req;
                s << "\n\n";
                client.write(s.str()).or_throw();
                client.shutdown().or_throw();
            }
            while (true);
        }
    };
}
