#pragma once

// TODO: use result<> for tokenizer return types over throwing exceptions

#include <txl/ref.h>
#include <txl/set.h>

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace txl::lexer
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
        
        auto read_one(int default_ch) -> int
        {
            if (empty())
            {
                return default_ch;
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
                if (not cond.process(*this, dst))
                {
                    return false;
                }
            }
            return true;
        }
        
        template<class Tok>
        auto skip_while(Tok && cond)
        {
            while (not empty() and cond(data_[next_]))
            {
                ++next_;
            }
        }
        
        template<class Tok>
        auto consume_until(Tok && cond, std::ostringstream & dst)
        {
            while (not empty() and not cond(data_[next_]))
            {
                if (not cond.process(*this, dst))
                {
                    return false;
                }
            }
            return true;
        }
        
        template<class Tok>
        auto skip_until(Tok && cond)
        {
            while (not empty() and not cond(data_[next_]))
            {
                ++next_;
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
                return true;
            }
            return false;
        }
    };

    struct token_tag
    {
        auto operator()(char ch) const -> bool
        {
            return true;
        }

        auto process(tokenizer & tok, std::ostringstream & dst) -> bool
        {
            auto ch = tok.read_one(-1);
            if (ch == -1)
            {
                return false;
            }
            dst.put(ch);
            return true;
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

        auto process(tokenizer & tok, std::ostringstream & dst) -> bool
        {
            auto ch = tok.peek(-1);
            if (ch == -1)
            {
                // End of buffer
                return false;
            }

            if (t1_(ch))
            {
                return t1_.process(tok, dst);
            }
            return t2_.process(tok, dst);
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
    
    class is_char : public token_tag
    {
    private:
        char allowed_;
    public:
        is_char(char allowed)
            : allowed_{allowed}
        {
        }

        auto operator()(char ch) const -> bool
        {
            return allowed_ == ch;
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
        ::txl::ref<TransformFunc> transform_;
    public:
        transform(Tok && tok, TransformFunc const & transformer)
            : tok_{std::move(tok)}
            , transform_{transformer}
        {
        }

        auto operator()(char ch) const -> bool
        {
            return tok_(ch);
        }

        auto process(tokenizer & tok, std::ostringstream & dst) -> bool
        {
            return (*transform_)(tok, dst);
        }
    };

    template<class Tok1, class Tok2, class = std::enable_if_t<std::is_base_of_v<token_tag, Tok1> and std::is_base_of_v<token_tag, Tok2>>>
    inline auto operator|(Tok1 && t1, Tok2 && t2) -> or_token<Tok1, Tok2>
    {
        return {std::move(t1), std::move(t2)};
    }
}
