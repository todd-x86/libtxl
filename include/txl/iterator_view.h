#pragma once

#include <iterator>

namespace txl
{
    template<class Iter>
    class iterator_view
    {
    private:
        Iter begin_{};
        Iter end_{};
    public:
        iterator_view() noexcept = default;

        template<class CIter>
        iterator_view(CIter begin, CIter end) noexcept
            : begin_(begin)
            , end_(end)
        {
        }

        template<class Callback>
        auto foreach(Callback && callback) -> void
        {
            for (auto const & el : *this)
            {
                callback(el);
            }
        }

        auto begin() noexcept -> Iter { return begin_; }
        auto end() noexcept -> Iter { return end_; }
        auto begin() const noexcept -> Iter { return begin_; }
        auto end() const noexcept -> Iter { return end_; }
        auto size() const noexcept -> size_t { return std::distance(begin_, end_); }
        auto empty() const noexcept -> bool { return begin_ == end_; }
        auto from_begin(int offset) const -> iterator_view
        {
            auto begin = begin_;
            while (offset)
            {
                ++begin;
                --offset;
            }
            return {begin, end_};
        }
        
        auto trim_end(int offset) const -> iterator_view
        {
            auto end = end_;
            while (offset and end != begin_)
            {
                ++offset;
                --end;
            }
            return {begin_, end};
        }
    };

    template<class Iter>
    inline auto make_iterator_view(Iter begin, Iter end) noexcept -> iterator_view<Iter>
    {
        return iterator_view<Iter>{begin, end};
    }

    template<class Iter>
    inline auto make_empty_iterator_view(Iter end) noexcept -> iterator_view<Iter>
    {
        return iterator_view<Iter>{end};
    }
}
