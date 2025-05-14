#pragma once

#include <iterator>
#include <type_traits>

namespace txl
{
    template<class Iter>
    class circular_iterator
    {
    private:
        Iter begin_;
        Iter end_;
        Iter next_;

        template<class It>
        circular_iterator(It begin, It end, It next)
            : begin_(begin)
            , end_(end)
            , next_(next)
        {
        }

        auto prev()
        {
            if (next_ == begin_)
            {
                next_ = end_;
            }
            --next_;
        }

        auto next()
        {
            ++next_;
            if (next_ == end_)
            {
                next_ = begin_;
            }
        }
    public:
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using reference = typename std::iterator_traits<Iter>::reference;
        using pointer = typename std::iterator_traits<Iter>::pointer;
        using difference_type = typename std::iterator_traits<Iter>::difference_type;
        using iterator_category = typename std::iterator_traits<Iter>::iterator_category;

        template<class It>
        circular_iterator(It begin, It end)
            : circular_iterator(begin, end, begin)
        {
        }

        auto operator*() -> reference
        {
            return *next_;
        }

        auto operator->() -> pointer
        {
            return &(*next_);
        }
        
        auto operator--() -> circular_iterator &
        {
            prev();
            return *this;
        }
        
        auto operator--(int) -> circular_iterator &
        {
            prev();
            return *this;
        }

        auto operator++() -> circular_iterator &
        {
            next();
            return *this;
        }

        auto operator++(int) -> circular_iterator &
        {
            next();
            return *this;
        }
        
        auto operator+(difference_type i) -> circular_iterator
        {
            auto it = circular_iterator{begin_, end_, next_};
            it += i;
            return it;
        }

        auto operator+=(difference_type i) -> circular_iterator &
        {
            while (i != 0)
            {
                auto dist = std::min(i, std::distance(next_, end_));
                next_ += dist;
                i -= dist;
                if (next_ == end_)
                {
                    next_ = begin_;
                }
            }
            return *this;
        }
        
        auto operator-(circular_iterator const & it) -> difference_type
        {
            auto my_pos = std::distance(begin_, next_);
            auto their_pos = std::distance(it.begin_, it.next_);
            if (my_pos <= their_pos)
            {
                return their_pos - my_pos;
            }
            auto total_dist = std::distance(begin_, end_);
            return their_pos + total_dist - my_pos;
        }
        
        auto operator-(difference_type i) -> circular_iterator
        {
            auto it = circular_iterator{begin_, end_, next_};
            it -= i;
            return it;
        }
        
        auto operator-=(difference_type i) -> circular_iterator &
        {
            while (i != 0)
            {
                if (next_ == begin_)
                {
                    next_ = end_;
                }
                auto dist = std::min(i, std::distance(begin_, next_));
                next_ -= dist;
                i -= dist;
            }
            return *this;
        }
    };

    template<class Value>
    class basic_iterator final
    {
    private:
        Value * current_;
    public:
        basic_iterator(Value * current)
            : current_{current}
        {
        }

        auto operator->() const -> Value *
        {
            return current_;
        }

        auto operator*() const -> Value &
        {
            return *current_;
        }

        auto operator++() -> basic_iterator &
        {
            ++current_;
            return *this;
        }

        auto operator++(ptrdiff_t d) -> basic_iterator &
        {
            current_ += d;
            return *this;
        }

        auto operator--() -> basic_iterator &
        {
            --current_;
            return *this;
        }

        auto operator--(ptrdiff_t d) -> basic_iterator &
        {
            current_ -= d;
            return *this;
        }
    };
    
    template<class Iter>
    inline auto make_circular_iterator(Iter begin, Iter end) noexcept -> circular_iterator<Iter>
    {
        return {begin, end};
    }

    // This differs from std::copy_n() since std::copy_n() simply calls std::copy() with
    // the input iterator N distance ahead, which in the case of special iterators like
    // circular iterator, this principle gets violated when you iterate more than the
    // entire distance of the iterator.
    template<class InputIter, class OutputIter>
    inline auto iter_copy_n(InputIter in, size_t n, OutputIter out) -> void
    {
        while (n != 0)
        {
            *out = *in;
            ++out;
            ++in;
            --n;
        }
    }
}
