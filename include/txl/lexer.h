#pragma once

#include <txl/set.h>
#include <stdexcept>
#include <string_view>

namespace txl
{
    namespace lexer
    {
        struct expect_error : std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        struct eof_error : std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };
        
        class tokenizer
        {
        private:
            std::string_view data_;
            size_t next_ = 0;
        public:
            tokenizer() = default;
            tokenizer(std::string_view data, size_t next = 0)
                : data_{data}
                , next_{next}
            {
            }

            auto read_one() -> char
            {
                if (empty())
                {
                    throw eof_error{"end of buffer"};
                }
                auto ch = data_[next_];
                ++next_;
                return ch;
            }

            auto empty() const -> bool
            {
                return next_ >= data_.size();
            }

            auto try_expect(char ch) -> bool
            {
                if (empty())
                {
                    return false;
                }
                if (data_[next_] != ch)
                {
                    return false;
                }
                ++next_;
                return true;
            }

            auto expect(char ch)
            {
                if (empty())
                {
                    throw eof_error{"end of buffer"};
                }
                if (data_[next_] != ch)
                {
                    throw expect_error{"character mismatch"};
                }
                ++next_;
            }

            template<class Tok>
            auto consume_while(Tok && cond, std::ostringstream & dst)
            {
                while (not empty() and cond(data_[next_]))
                {
                    cond.process(*this, dst);
                }
            }
            
            auto peek(int default_ch) const -> int
            {
                if (empty())
                {
                    return default_ch;
                }
                return data_[next_];
            }

            auto skip()
            {
                if (not empty())
                {
                    ++next_;
                }
            }
        };

        struct token_tag
        {
            auto operator()(char ch) const -> bool
            {
                return true;
            }

            auto process(tokenizer & tok, std::ostringstream & dst) -> void
            {
                dst.put(tok.read_one());
            }
        };

        template<class T1, class T2>
        class or_token : public token_tag
        {
        private:
            T1 t1_;
            T2 t2_;
        public:
            or_token(T1 && t1, T2 && t2)
                : t1_{std::move(t1)}
                , t2_{std::move(t2)}
            {
            }

            auto operator()(char ch) const -> bool
            {
                return t1_(ch) or t2_(ch);
            }

            auto process(tokenizer & tok, std::ostringstream & dst) -> void
            {
                auto ch = tok.peek(-1);
                if (ch == -1)
                {
                    // End of buffer
                    return;
                }

                if (t1_(ch))
                {
                    t1_.process(tok, dst);
                    return;
                }
                t2_.process(tok, dst);
            }
        };

        class in_range : public token_tag
        {
        private:
            char begin_;
            char end_;
        public:
            in_range(char begin, char inc_end)
                : begin_{begin}
                , end_{inc_end}
            {
            }

            auto operator()(char ch) const -> bool
            {
                return ch >= begin_ and ch <= end_;
            }
        };

        class in_set : public token_tag
        {
        private:
            ::txl::set<char> accepted_;
        public:
            in_set(std::initializer_list<char> && accepted)
                : accepted_{std::move(accepted)}
            {
            }

            auto operator()(char ch) const -> bool
            {
                return accepted_.contains(ch);
            }
        };
        
        template<class Tok, class TransformFunc>
        class transform : public token_tag
        {
        private:
            Tok tok_;
            TransformFunc transform_;
        public:
            transform(Tok && tok, TransformFunc transformer)
                : tok_{std::move(tok)}
                , transform_{transformer}
            {
            }

            auto operator()(char ch) const -> bool
            {
                return tok_(ch);
            }

            auto process(tokenizer & tok, std::ostringstream & dst) -> void
            {
                transform_(tok, dst);
            }
        };

        template<class Tok1, class Tok2, class = std::enable_if_t<std::is_base_of_v<token_tag, Tok1> and std::is_base_of_v<token_tag, Tok2>>>
        inline auto operator|(Tok1 && t1, Tok2 && t2) -> or_token<Tok1, Tok2>
        {
            return {std::move(t1), std::move(t2)};
        }
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

    struct hex_error : std::runtime_error
    {
        using std::runtime_error::runtime_error;
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
        };

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
                throw hex_error{"non-hexadecimal character received"};
            }

            auto operator()(lexer::tokenizer & tok, std::ostringstream & dst) -> void
            {
                // Got '%'
                tok.skip();

                auto hex1 = tok.read_one();
                auto hex2 = tok.read_one();

                char decoded = (hex_to_int(hex1) << 4) | hex_to_int(hex2);

                dst.put(decoded);
            }
        };

        state state_ = S0;
        std::ostringstream name_;
        std::ostringstream value_;
        query_param_notifier * notif_;

        auto parse_state_0(lexer::tokenizer & tok)
        {
            tok.expect('?');
            state_ = S1;
            name_.str("");
        }

        auto reset(state s)
        {
            name_.str("");
            value_.str("");
            state_ = s;
        }

        auto consume_tokens(lexer::tokenizer & tok, std::ostringstream & dst)
        {
            using namespace ::txl::lexer;

            tok.consume_while(in_range{'0', '9'} 
                            | in_range{'A', 'Z'}
                            | in_range{'a', 'z'} 
                            | in_set{{'!', '$', '\'', '*', '-', '.', '^', '_', '`', '|', '~'}}
                            | transform{in_set({'%'}), hex_transformer{}}
                            | transform{in_set({'+'}), [](auto & tok, auto & dst) { tok.skip(); dst.put(' '); }}
                            , dst);
        }

        auto notify()
        {
            if (name_.tellp() == 0)
            {
                return;
            }
            notif_->on_param(query_param{name_.str(), value_.str()});
        }

        auto parse_state_1(lexer::tokenizer & tok)
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
                    notify();
                    reset(S1);
                    break;
                default:
                    notify();
                    reset(S3);
                    break;
            }
        }
        
        auto parse_state_2(lexer::tokenizer & tok)
        {
            consume_tokens(tok, value_);
            switch (tok.peek(-1))
            {
                case '&':
                    tok.skip();
                    notify();
                    reset(S1);
                    break;
                default:
                    notify();
                    reset(S3);
                    break;
            }
        }
    public:
        query_string_parser(query_param_notifier & notif)
            : notif_{&notif}
        {
        }

        auto parse(lexer::tokenizer & tok)
        {
            while (not tok.empty())
            {
                switch (state_)
                {
                    case S0:
                        parse_state_0(tok);
                        break;
                    case S1:
                        parse_state_1(tok);
                        break;
                    case S2:
                        parse_state_2(tok);
                        break;
                    case S3:
                        return;
                }
            }
        }
    };
}
