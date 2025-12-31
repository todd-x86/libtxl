#pragma once

#include <txl/buffer_ref.h>
#include <txl/lexer.h>
#include <txl/set.h>
#include <txl/socket_address.h>
#include <txl/socket.h>
#include <txl/result.h>
#include <txl/read_string.h>
#include <txl/size_policy.h>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace txl::http
{
    struct url_decode_error : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    namespace detail
    {
        struct hex_transformer final
        {
            auto hex_to_int(char c) -> int
            {
                if (c >= '0' and c <= '9')
                {
                    return c - '0';
                }
                if (c >= 'a' and c <= 'f')
                {
                    return 10 + (c - 'a');
                }
                if (c >= 'A' and c <= 'F')
                {
                    return 10 + (c - 'A');
                }
                throw url_decode_error{"non-hexadecimal character received"};
            }

            auto operator()(::txl::lexer::tokenizer & tok, std::ostringstream & dst) -> void
            {
                // Got '%'
                tok.skip();

                auto hex1 = tok.read_one();
                auto hex2 = tok.read_one();

                char decoded = (hex_to_int(hex1) << 4) | hex_to_int(hex2);

                dst.put(decoded);
            }
        };
    }
    
    struct query_param
    {
        std::string name;
        std::string value;

        auto operator==(query_param const & p) const -> bool
        {
            return name == p.name and value == p.value;
        }
    };

    inline auto operator<<(std::ostream & os, query_param const & p) -> std::ostream &
    {
        os << "(" << p.name << "=" << p.value << ")";
        return os;
    }

    struct query_param_notifier
    {
        virtual auto on_param(query_param param) -> void = 0;
    };

    struct query_param_vector : std::vector<query_param>
                              , query_param_notifier
    {
        using std::vector<query_param>::vector;

        auto on_param(query_param param) -> void override
        {
            this->emplace_back(std::move(param));
        }
    };

    class query_string_parser
    {
    private:
        enum state
        {
            S0, // <start> ('?')
            S1, // <name> ('=' -> S2, '&' -> S1)
            S2, // <val> ('&' -> S1)
            S3, // <end>
            S4, // <error>
        };


        state state_ = S0;
        std::ostringstream name_;
        std::ostringstream value_;

        auto parse_state_0(::txl::lexer::tokenizer & tok)
        {
            if (not tok.try_expect('?'))
            {
                state_ = S4;
                return;
            }
            state_ = S1;
            name_.str("");
        }

        auto reset(state s)
        {
            name_.str("");
            value_.str("");
            state_ = s;
        }

        auto consume_tokens(::txl::lexer::tokenizer & tok, std::ostringstream & dst)
        {
            using namespace ::txl::lexer;

            tok.consume_while(in_range{'0', '9'} 
                            | in_range{'A', 'Z'}
                            | in_range{'a', 'z'} 
                            | in_set{{'!', '$', '\'', '*', '-', '.', '^', '_', '`', '|', '~'}}
                            | transform{in_set({'%'}), detail::hex_transformer{}}
                            | transform{in_set({'+'}), [](auto & tok, auto & dst) { tok.skip(); dst.put(' '); }}
                            , dst);
        }

        auto notify(query_param_notifier & notif)
        {
            if (name_.tellp() != 0)
            {
                notif.on_param({name_.str(), value_.str()});
            }
        }

        auto parse_state_1(::txl::lexer::tokenizer & tok, query_param_notifier & notif)
        {
            consume_tokens(tok, name_);
            switch (tok.peek(-1))
            {
                case '=':
                    tok.skip();
                    state_ = S2;
                    break;
                case '&':
                case '?':
                    tok.skip();
                    notify(notif);
                    reset(S1);
                    break;
                default:
                    notify(notif);
                    reset(S3);
                    break;
            }
        }
        
        auto parse_state_2(::txl::lexer::tokenizer & tok, query_param_notifier & notif)
        {
            consume_tokens(tok, value_);
            switch (tok.peek(-1))
            {
                case '&':
                    tok.skip();
                    notify(notif);
                    reset(S1);
                    break;
                default:
                    notify(notif);
                    reset(S3);
                    break;
            }
        }
    public:
        auto is_error() const -> bool
        {
            return state_ == S4;
        }

        auto parse(::txl::lexer::tokenizer & tok, query_param_notifier & notif)
        {
            while (not tok.empty())
            {
                switch (state_)
                {
                    case S0:
                        parse_state_0(tok);
                        break;
                    case S1:
                        parse_state_1(tok, notif);
                        break;
                    case S2:
                        parse_state_2(tok, notif);
                        break;
                    case S3:
                    case S4:
                        return;
                }
            }
        }
    };

    struct http_header_field final
    {
        std::string name;
        std::string value;
    };
    
    inline auto operator<<(std::ostream & os, http_header_field const & field) -> std::ostream &
    {
        os << field.name << ": " << field.value;
        return os;
    }

    struct http_request
    {
        std::string request_type;
        std::string uri;
        query_param_vector query_params;
        std::string http_version;
        std::vector<http_header_field> headers;
    };

    inline auto operator<<(std::ostream & os, http_request const & req) -> std::ostream &
    {
        os << "{ request_type=" << req.request_type << ", uri=" << req.uri << ", query_params=[";
        for (auto const & param : req.query_params)
        {
            os << " " << param << ", ";
        }
        os << "], http_version=" << req.http_version << ", headers=[";
        for (auto const & field : req.headers)
        {
            os << " \"" << field << "\", ";
        }
        os << "] }";
        return os;
    }

    class http_1_1_parser
    {
    public:
        enum class state
        {
            REQUEST_TYPE, // e.g. "GET"
            URI, // e.g. "/foo"
            QUERY_PARAM, // e.g. "?id=123"
            HTTP_VERSION, // e.g. "HTTP/1.1"
            REQUEST_CR, // "\r"
            REQUEST_LF, // "\n"
            HEADER_KEY, // e.g. "Host"
            HEADER_VALUE, // e.g. "nuradius.com"
            HEADER_CR, // "\r"
            HEADER_LF, // "\n"
            END_CR, // "\r"
            END_LF, // "\n"
            PARSER_COMPLETE,
            PARSER_ERROR,
        };
    private:
        http_request stg_;
        query_string_parser query_string_parser_;
        state state_ = state::REQUEST_TYPE;
        bool new_header_ = true;

        auto stg_header() -> http_header_field &
        {
            if (new_header_)
            {
                stg_.headers.emplace_back();
                new_header_ = false;
            }
            return stg_.headers.back();
        }
    public:
        auto is_complete() const -> bool
        {
            return state_ == state::PARSER_COMPLETE or has_error();
        }
        
        auto has_error() const -> bool
        {
            return state_ == state::PARSER_ERROR;
        }

        auto request() const -> http_request const &
        {
            return stg_;
        }

        auto request() -> http_request &
        {
            return stg_;
        }

        auto process(std::string_view data) -> size_t
        {
            using namespace txl::lexer;

            std::ostringstream tok_buf{};
            auto tok = ::txl::lexer::tokenizer{data};
            while (not tok.empty())
            {
                switch (state_)
                {
                    case state::REQUEST_TYPE:
                    {
                        tok_buf.str("");
                        tok.consume_until( in_set({' ', '\r', '\n'}), tok_buf );
                        stg_.request_type.append(tok_buf.str());
                        switch (tok.peek(-1))
                        {
                            case ' ':
                                tok.skip();
                                state_ = state::URI;
                                break;
                            case -1:
                                break;
                            default:
                                state_ = state::PARSER_ERROR;
                                break;
                        }
                        break;
                    }
                    case state::URI:
                    {
                        tok_buf.str("");
                        tok.consume_until( in_set({' ', '\r', '\n', '?'}), tok_buf );
                        stg_.uri.append(tok_buf.str());
                        switch (tok.peek(-1))
                        {
                            case '?':
                                state_ = state::QUERY_PARAM;
                                break;
                            case ' ':
                                tok.skip();
                                state_ = state::HTTP_VERSION;
                                break;
                            case -1:
                                break;
                            default:
                                state_ = state::PARSER_ERROR;
                                break;
                        }
                        break;
                    }
                    case state::QUERY_PARAM:
                    {
                        query_string_parser_.parse(tok, stg_.query_params);
                        if (query_string_parser_.is_error())
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        else
                        {
                            state_ = state::HTTP_VERSION;
                        }
                        break;
                    }
                    case state::HTTP_VERSION:
                    {
                        tok_buf.str("");
                        tok.consume_until( in_set({'\r', '\n'}), tok_buf );
                        stg_.http_version.append(tok_buf.str());
                        switch (tok.peek(-1))
                        {
                            case '\r':
                                tok.skip();
                                state_ = state::REQUEST_CR;
                                break;
                            case '\n':
                                tok.skip();
                                state_ = state::REQUEST_LF;
                                break;
                            case -1:
                                break;
                        }
                        break;
                    }
                    case state::REQUEST_CR:
                    {
                        if (tok.empty())
                        {
                            break;
                        }
                        else if (tok.try_expect('\r'))
                        {
                            state_ = state::REQUEST_LF;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::REQUEST_LF:
                    {
                        if (tok.empty())
                        {
                            break;
                        }
                        else if (tok.try_expect('\n'))
                        {
                            state_ = state::HEADER_KEY;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::HEADER_KEY:
                    {
                        tok_buf.str("");
                        tok.consume_until( in_set({':', '\r', '\n'}), tok_buf );
                        stg_header().name.append(tok_buf.str());

                        switch (tok.peek(-1))
                        {
                            case '\r':
                                tok.skip();
                                state_ = state::END_CR;
                                break;
                            case '\n':
                                tok.skip();
                                state_ = state::PARSER_COMPLETE;
                                break;
                            case ':':
                                tok.skip();
                                tok.skip_while(is_char{' '});
                                state_ = state::HEADER_VALUE;
                                break;
                            case -1:
                                break;
                        }
                        break;
                    }
                    case state::HEADER_VALUE:
                    {
                        tok_buf.str("");
                        tok.consume_until( in_set({'\r', '\n'}), tok_buf );
                        stg_header().value.append(tok_buf.str());
                        switch (tok.peek(-1))
                        {
                            case '\r':
                                tok.skip();
                                state_ = state::HEADER_CR;
                                break;
                            case '\n':
                                tok.skip();
                                state_ = state::HEADER_KEY;
                                new_header_ = true;
                                break;
                            case -1:
                                break;
                        }
                        break;
                    }
                    case state::HEADER_CR:
                    {
                        if (tok.empty())
                        {
                            break;
                        }
                        else if (tok.try_expect('\r'))
                        {
                            state_ = state::HEADER_LF;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::HEADER_LF:
                    {
                        if (tok.empty())
                        {
                            break;
                        }
                        else if (tok.try_expect('\n'))
                        {
                            state_ = state::HEADER_KEY;
                            new_header_ = true;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::END_CR:
                    {
                        if (tok.empty())
                        {
                            break;
                        }
                        else if (tok.try_expect('\r'))
                        {
                            state_ = state::END_LF;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::END_LF:
                    {
                        if (tok.empty())
                        {
                            break;
                        }
                        else if (tok.try_expect('\n'))
                        {
                            state_ = state::PARSER_COMPLETE;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::PARSER_COMPLETE:
                        return {};
                    case state::PARSER_ERROR:
                        // Failure case
                        return {};
                }
            }
            return {};
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
                listener_.listen(0).or_throw();

                auto client = listener_.accept().or_throw();
                auto req = ::txl::read_string(client, ::txl::one_of{::txl::until{"\n\n"}, ::txl::at_most{1024}}).or_throw();
                auto http_parser = http_1_1_parser{};
                http_parser.process(req);

                auto s = std::ostringstream{};
                s << "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<h1>Hello</h1>";
                s << "<pre>" << http_parser.request() << "</pre>";
                s << "<pre>" << req << "</pre>";
                s << "\n\n";
                client.write(s.str()).or_throw();
                client.shutdown().or_throw();
            }
            while (true);
        }
    };
}
