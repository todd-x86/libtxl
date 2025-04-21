#pragma once

#include <txl/buffer_ref.h>
#include <txl/socket_address.h>
#include <txl/socket.h>
#include <txl/result.h>
#include <txl/read_string.h>
#include <txl/size_policy.h>
#include <iostream>
#include <optional>
#include <string>

namespace txl::http
{
    struct http_header_field final
    {
        ::txl::buffer_ref name;
        ::txl::buffer_ref value;
    };

    class http_1_1_parser
    {
    private:
        auto parse_http_request() -> void
        {
            parse_start_line();
            std::optional<http_header_field> header{};
            do
            {
                header = parse_field_line();
            }
            while (header);
            parse_message_body();
        }

        auto parse_start_line() -> void
        {
        }

        auto parse_field_line() -> std::optional<http_header_field>
        {
            return {};
        }

        auto parse_message_body() -> void
        {
        }
    };
    
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
