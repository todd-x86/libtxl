#pragma once

#include <iterator>

namespace txl {

template<class Iter>
class iterator_view
{
private:
    Iter begin_;
    Iter end_;
public:
    template<class CIter>
    iterator_view(CIter begin, CIter end)
        : begin_(begin)
        , end_(end)
    {
    }

    auto begin() -> Iter { return begin_; }
    auto end() -> Iter { return end_; }
    auto begin() const -> Iter { return begin_; }
    auto end() const -> Iter { return end_; }
    auto size() const -> size_t { return std::distance(begin_, end_); }
    auto empty() const -> bool { return begin_ == end_; }
};

template<class Iter>
inline auto make_iterator_view(Iter begin, Iter end) -> iterator_view<Iter>
{
    return iterator_view<Iter>{begin, end};
}

template<class Iter>
inline auto make_empty_iterator_view(Iter end) -> iterator_view<Iter>
{
    return iterator_view<Iter>{end};
}

}
