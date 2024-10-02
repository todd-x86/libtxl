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
    public:
        using value_type = std::iterator_traits<Iter>::value_type;
        using reference = std::iterator_traits<Iter>::reference;
        using pointer = std::iterator_traits<Iter>::pointer;
        using difference_type = std::iterator_traits<Iter>::difference_type;
        using iterator_category = std::iterator_traits<Iter>::iterator_category;

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
            if (next_ == begin_)
            {
                next_ = end_;
            }
            --next_;
            return *this;
        }

        auto operator++() -> circular_iterator &
        {
            ++next_;
            if (next_ == end_)
            {
                next_ = begin_;
            }
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
            // I think this is right???
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
    
    template<class Iter>
    inline auto make_circular_iterator(Iter begin, Iter end) noexcept -> circular_iterator<Iter>
    {
        return {begin, end};
    }
}
