#pragma once

#include <txl/buffer_ref.h>
#include <txl/set.h>
#include <txl/socket_address.h>
#include <txl/socket.h>
#include <txl/result.h>
#include <txl/read_string.h>
#include <txl/size_policy.h>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace txl::http
{
    class lexer final
    {
    private:
        std::string_view data_;
        size_t index_;
    public:
        struct status final
        {
            std::string_view lexed;
            char last_match = '\0';
            bool complete = false;
        };

        class is final
        {
        private:
            ::txl::set<char> allowed_;
        public:
            is(::txl::set<char> && allowed)
                : allowed_{std::move(allowed)}
            {
            }

            auto operator()(char ch) const -> bool
            {
                return allowed_.contains(ch);
            }
        };
        
        class is_not final
        {
        private:
            ::txl::set<char> not_allowed_;
        public:
            is_not(::txl::set<char> && not_allowed)
                : not_allowed_{std::move(not_allowed)}
            {
            }

            auto operator()(char ch) const -> bool
            {
                return not not_allowed_.contains(ch);
            }
        };

        struct any final
        {
            auto operator()(char ch) const -> bool
            {
                return true;
            }
        };

        class exactly final
        {
        private:
            size_t num_read_ = 0;
            size_t num_to_read_;
        public:
            exactly(size_t num_to_read)
                : num_to_read_{num_to_read}
            {
            }

            auto operator()(char ch) -> bool
            {
                ++num_read_;
                return num_read_ <= num_to_read_;
            }
        };

        lexer(std::string_view data, size_t index = 0)
            : data_{data}
            , index_{index}
        {
        }

        auto is_complete() const -> bool
        {
            return index_ >= data_.size();
        }

        template<class Cond>
        auto read(Cond && cond, std::optional<size_t> min = {}, std::optional<size_t> max = {}) -> status
        {
            auto last = index_;
            while (index_ < data_.size() and cond(data_[index_]))
            {
                ++index_;
            }
            auto dist = index_ - last;
            if (min.has_value() and dist < *min)
            {
                index_ = last;
                return {};
            }
            if (max.has_value() and dist > *max)
            {
                index_ = last;
                return {};
            }

            // `complete` is driven by making it to the condition
            auto last_match = '\0';
            if (index_ < data_.size())
            {
                last_match = data_[index_];
            }
            return {data_.substr(last, index_-last), last_match, (last != index_ and index_ < data_.size())};
        }

        auto read_one(char c) -> status
        {
            return read(is({c}), 1, 1);
        }

        auto skip(size_t num_chars) -> status
        {
            return read(exactly(num_chars));
        }
        
        template<class Cond>
        auto peek(Cond && cond) const -> status
        {
            auto idx = index_;
            while (idx < data_.size() and cond(data_[idx]))
            {
                ++idx;
            }

            // `complete` is driven by making it to the condition
            auto last_match = '\0';
            if (idx < data_.size())
            {
                last_match = data_[idx];
            }
            return {data_.substr(index_, idx-index_), last_match, (index_ != idx and idx < data_.size())};
        }
    };

    struct http_header_field final
    {
        std::string name;
        std::string value;

        http_header_field() = default;
    };
    
    inline auto operator<<(std::ostream & os, http_header_field const & field) -> std::ostream &
    {
        os << field.name << ": " << field.value;
        return os;
    }
    
    struct http_query_param final
    {
        std::string name;
        std::string value;

        http_query_param() = default;
    };
    
    inline auto operator<<(std::ostream & os, http_query_param const & param) -> std::ostream &
    {
        os << param.name << "=" << param.value;
        return os;
    }

    struct http_request
    {
        std::string request_type;
        std::string uri;
        std::vector<http_query_param> query_params;
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
            REQUEST_TYPE_SPACE, // " "
            URI, // e.g. "/foo"
            QUERY_PARAM_NAME, // e.g. "id"
            QUERY_PARAM_EQU, // '='
            QUERY_PARAM_VALUE, // e.g. "123"
            HTTP_VERSION, // e.g. "HTTP/1.1"
            REQUEST_CR, // "\r"
            REQUEST_LF, // "\n"
            HEADER_KEY, // e.g. "Host"
            HEADER_COL, // ":"
            HEADER_SPACE, // " "
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
            auto lex = lexer{data};
            while (not lex.is_complete())
            {
                switch (state_)
                {
                    case state::REQUEST_TYPE:
                    {
                        auto res = lex.read( lexer::is_not({' ', '\r', '\n'}) );
                        stg_.request_type.append(res.lexed);
                        if (res.complete)
                        {
                            state_ = state::REQUEST_TYPE_SPACE;
                        }
                        break;
                    }
                    case state::REQUEST_TYPE_SPACE:
                    {
                        auto res = lex.read_one(' ');
                        if (res.complete)
                        {
                            state_ = state::URI;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::URI:
                    {
                        auto res = lex.read( lexer::is_not({' ', '\r', '\n', '?'}) );
                        stg_.uri.append(res.lexed);
                        if (res.complete)
                        {
                            switch (res.last_match)
                            {
                                case '?':
                                    lex.skip(1);
                                    state_ = state::QUERY_PARAM_NAME;
                                    break;
                                case ' ':
                                    lex.skip(1);
                                    state_ = state::HTTP_VERSION;
                                    break;
                                default:
                                    state_ = state::PARSER_ERROR;
                                    break;
                            }
                        }
                        break;
                    }
                    case state::QUERY_PARAM_NAME:
                    {
                        break;
                    }
                    case state::QUERY_PARAM_EQU:
                    {
                        break;
                    }
                    case state::QUERY_PARAM_VALUE:
                    {
                        break;
                    }
                    case state::HTTP_VERSION:
                    {
                        auto res = lex.read( lexer::is_not({' ', '\r', '\n'}) );
                        stg_.http_version.append(res.lexed);
                        if (res.complete)
                        {
                            state_ = state::REQUEST_CR;
                        }
                        break;
                    }
                    case state::REQUEST_CR:
                    {
                        auto res = lex.read_one('\r');
                        if (res.complete)
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
                        auto res = lex.read_one('\n');
                        if (res.complete)
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
                        auto res = lex.read( lexer::is_not({' ', ':', '\r', '\n'}) );
                        if (res.complete and lex.peek(lexer::is({'\r'})).complete)
                        {
                            state_ = state::END_CR;
                            break;
                        }
                        stg_header().name.append(res.lexed);
                        if (res.complete)
                        {
                            state_ = state::HEADER_COL;
                        }
                        break;
                    }
                    case state::HEADER_COL:
                    {
                        auto res = lex.read_one(':');
                        if (res.complete)
                        {
                            state_ = state::HEADER_SPACE;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::HEADER_SPACE:
                    {
                        auto res = lex.read_one(' ');
                        if (res.complete)
                        {
                            state_ = state::HEADER_VALUE;
                        }
                        else
                        {
                            state_ = state::PARSER_ERROR;
                        }
                        break;
                    }
                    case state::HEADER_VALUE:
                    {
                        auto res = lex.read( lexer::is_not({'\r', '\n'}) );
                        stg_header().value.append(res.lexed);
                        if (res.complete)
                        {
                            state_ = state::HEADER_CR;
                        }
                        break;
                    }
                    case state::HEADER_CR:
                    {
                        auto res = lex.read_one('\r');
                        if (res.complete)
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
                        auto res = lex.read_one('\n');
                        if (res.complete)
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
                        auto res = lex.read_one('\r');
                        if (res.complete)
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
                        auto res = lex.read_one('\n');
                        // NOTE: cannot check `res.complete` because that expects us to not reach EOF
                        if (res.last_match == '\n')
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
