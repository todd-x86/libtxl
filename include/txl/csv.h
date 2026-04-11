#pragma once

#include <txl/io.h>
#include <txl/buffer_ref.h>
#include <txl/copy.h>
#include <txl/size_policy.h>
#include <txl/result.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <cstdlib>
#include <vector>
#include <ostream>
#include <stdexcept>
#include <sstream>

namespace txl::csv
{
    enum class splitter_status
    {
        add,
        more_data,

        delimiter,
        no_op,
    };

    template<class CharType = char>
    class splitter final
    {
    private:
        static constexpr const CharType QUOTE = static_cast<CharType>('"');

        /**
         * foo,bar
         * ^  ^^  ^
         * |  ||  |
         * |  ||  +- EOL
         * |  ||
         * |  |+- data
         * |  +-- delimiter
         * +----- data
         *
         * "foo","bar"
         * ^^  ^^^^  ^^
         * ||  ||||  ||
         * ||  ||||  |+- EOL
         * ||  ||||  +-- end_quote
         * ||  |||+----- quote_data
         * ||  ||+------ begin_quote
         * ||  |+------- delimiter
         * ||  +-------- end_quote
         * |+----------- quote_data
         * +------------ begin_quote
         *
         * "fo""o","b""ar"
         * ^^ ^^^^^^^^^^ ^^
         * || |||||||||| ||
         * || |||||||||| |+- EOL
         * || |||||||||| +-- end_quote
         * || |||||||||+---- quote_data
         * || ||||||||+----- quote_char
         * || |||||||+------ end_quote
         * || ||||||+------- quote_data
         * || |||||+-------- begin_quote
         * || ||||+--------- delimiter
         * || |||+---------- end_quote
         * || ||+----------- quote_data
         * || |+------------ quote_char
         * || +------------- end_quote
         * |+--------------- quote_data
         * +---------------- begin_quote
         *
         */
        enum class state
        {
            begin, // (beginning)
            data, // -> delimiter | EOL
            begin_quote, // -> quote_data | end_quote
            quote_data, // -> end_quote
            end_quote, // -> delimiter | quote_char | EOL
            delimiter, // -> begin_quote | data | EOL
        };

        CharType delim_;
        state state_ = state::begin;
    public:
        splitter(CharType delimiter = static_cast<CharType>(','))
            : delim_{delimiter}
        {
        }

        auto reset() -> void
        {
            state_ = state::begin;
        }

        auto push(CharType ch) -> splitter_status
        {
            switch (state_)
            {
                case state::begin:
                    if (ch == QUOTE)
                    {
                        // Quoted data
                        state_ = state::quote_data;
                        return splitter_status::add;
                    }
                    if (ch == delim_)
                    {
                        // No data
                        state_ = state::delimiter;
                        return splitter_status::delimiter;
                    }

                    // Regular data
                    state_ = state::data;
                    return splitter_status::add;
                case state::data:
                    if (ch != delim_)
                    {
                        return splitter_status::add;
                    }
                    state_ = state::delimiter;
                    return splitter_status::delimiter;
                case state::begin_quote:
                    if (ch != QUOTE)
                    {
                        state_ = state::quote_data;
                        return splitter_status::add;
                    }
                    state_ = state::end_quote;
                    return splitter_status::more_data;
                case state::quote_data:
                    if (ch != QUOTE)
                    {
                        return splitter_status::add;
                    }
                    state_ = state::end_quote;
                    return splitter_status::more_data;
                case state::end_quote:
                    if (ch == QUOTE)
                    {
                        // 2nd quote
                        state_ = state::quote_data;
                        return splitter_status::add;
                    }
                    if (ch == delim_)
                    {
                        state_ = state::delimiter;
                        return splitter_status::delimiter;
                    }

                    // Bad state, but we don't have an easy way to throw, so let's just keep moving along
                    state_ = state::data;
                    return splitter_status::add;
                case state::delimiter:
                    if (ch == QUOTE)
                    {
                        state_ = state::begin_quote;
                        return splitter_status::more_data;
                    }
                    state_ = state::data;
                    return splitter_status::add;
            }
            return splitter_status::no_op;
        }
    };
    
    enum class split_status
    {
        delimiter,
        end_of_line,
        empty,
    };    

    template<class CharType = char>
    class string_view_splitter final
    {
    public:
        using string_view = std::basic_string_view<CharType>;
    private:
        splitter<CharType> sp_;
        string_view str_;
    public:
        string_view_splitter(string_view s, CharType delim)
            : sp_{delim}
            , str_{s}
        {
        }

        auto reset(string_view s)
        {
            str_ = s;
            sp_.reset();
        }
        
        template<class CharFunc, class = std::enable_if_t<std::is_invocable_v<CharFunc, CharType>>>
        auto next(CharFunc && on_char) -> split_status
        {
            if (empty())
            {
                return split_status::empty;
            }

            auto has_delimiter = false;
            auto it = str_.begin();
            for (; it != str_.end() and not has_delimiter; ++it)
            {
                switch (sp_.push(*it))
                {
                    case splitter_status::add:
                        on_char(*it);
                        break;
                    case splitter_status::delimiter:
                        has_delimiter = true;
                        break;
                    case splitter_status::more_data:
                    default:
                        // No-op here
                        break;
                }
            }
            str_ = str_.substr(std::distance(str_.begin(), it));
            if (has_delimiter)
            {
                return split_status::delimiter;
            }
            return split_status::end_of_line;
        }

        auto next(std::basic_ostream<CharType> & dst) -> split_status
        {
            return next([&](auto ch) { dst.put(ch); });
        }

        auto next(std::vector<CharType> & dst) -> split_status
        {
            return next([&](auto ch) { dst.push_back(ch); });
        }

        auto empty() const -> bool
        {
            return str_.empty();
        }
    };

    template<class CharType = char, class RowFunc>
    auto parse_line(::txl::reader & src, RowFunc && dst) -> ::txl::result<size_t>
    {
        splitter<CharType> sp{};
        std::ostringstream buf{};

        // Keep the input buffer small by pushing directly to a lambda
        auto wr = ::txl::lambda_writer{[&](::txl::buffer_ref src) {
            for (auto ch : src.to_string_view<CharType>())
            {
                switch (sp.push(ch))
                {
                    case splitter_status::add:
                        buf.put(ch);
                        break;
                    case splitter_status::delimiter:
                        // Copy only when a delimiter is reached
                        dst(buf.str());
                        buf.str("");
                        break;
                    default:
                    case splitter_status::more_data:
                        break;
                }
            }
        }};

        auto res = ::txl::copy_until(src, wr, static_cast<CharType>('\n'));
        // Because we've been copying when we reach a delimiter, we need to copy the last column, reached by EOL
        if (buf.tellp() != 0)
        {
            dst(buf.str());
        }
        return res;
    }

    template<class CharType = char>
    using row = std::vector<std::basic_string<CharType>>;

    template<class CharType = char>
    class row_view final
    {
    private:
        row<CharType> const & row_;
        row<CharType> const * header_ = nullptr;
    public:
        row_view(row<CharType> const & row_data)
            : row_{row_data}
        {
        }
        
        row_view(row<CharType> const & row_data, row<CharType> const & header)
            : row_{row_data}
            , header_{&header}
        {
        }

        auto data() const -> row<CharType> const & { return row_; }

        auto size() const { return row_.size(); }

        auto operator[](size_t index) const -> std::basic_string<CharType> const &
        {
            return row_[index];
        }

        auto operator[](std::basic_string<CharType> const & key) const -> std::basic_string<CharType> const &
        {
            if (not header_)
            {
                throw std::runtime_error{"no header row specified"};
            }
            auto loc = std::find(header_->begin(), header_->end(), key);
            if (loc == header_->end())
            {
                throw std::runtime_error{"key not found"};
            }
            auto idx = static_cast<size_t>(std::distance(header_->begin(), loc));
            if (idx >= row_.size())
            {
                throw std::runtime_error{"index out-of-bounds"};
            }
            return (*this)[idx];
        }
    };
    
    template<class CharType = char>
    class document final
    {
    private:
        std::vector<row<CharType>> rows_;
        bool use_header_;
    public:
        document(bool use_header = false)
            : use_header_{use_header}
        {
        }

        document(std::initializer_list<row<CharType>> && rows, bool use_header = false)
            : rows_{std::move(rows)}
            , use_header_{use_header}
        {
        }

        auto operator[](size_t index) const -> row_view<CharType>
        {
            if (use_header_)
            {
                return row_view<CharType>{rows_[index+1], rows_[0]};
            }
            return row_view<CharType>{rows_[index]};
        }

        auto clear() -> void
        {
            rows_.clear();
        }

        auto emplace(size_t index, row<CharType> && r) -> void
        {
            rows_.emplace(std::next(rows_.begin(), index), std::move(r));
        }
        
        auto insert(size_t index, row<CharType> const & r) -> void
        {
            rows_.insert(std::next(rows_.begin(), index), r);
        }

        auto erase(size_t index) -> void
        {
            rows_.erase(std::next(rows_.begin(), index));
        }

        auto add(row<CharType> && r)
        {
            rows_.emplace_back(std::move(r));
        }

        auto add(row<CharType> const & r)
        {
            rows_.push_back(r);
        }

        auto size() const -> size_t
        {
            if (use_header_)
            {
                return rows_.size() - 1;
            }
            return rows_.size();
        }
    };

    template<class CharType = char>
    auto parse_document(::txl::reader & rd, document<CharType> & dst) -> ::txl::result<size_t>
    {
        size_t bytes_read = 0;
        while (true)
        {
            auto curr_row = row<CharType>{};
            auto res = parse_line(rd, [&](std::basic_string<CharType> && col) {
                curr_row.emplace_back(std::move(col));
            });

            if (not res)
            {
                return res.error();
            }
            if (*res == 0)
            {
                return bytes_read;
            }
            bytes_read += *res;

            dst.add(std::move(curr_row));
        }
    }
}
