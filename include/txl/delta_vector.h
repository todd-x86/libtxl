#pragma once

#include <vector>

namespace txl
{
    template<class Base, class Delta, class Value = decltype(std::declval<Base>() + std::declval<Delta>())>
    class delta_vector
    {
    private:
        Base base_;
        std::vector<Delta> deltas_;
    public:
        using value_type = Value;

        class const_iterator final
        {
        private:
            Base base_;
            Delta const * delta_;
        public:
            const_iterator(Base const & base, Delta const * delta)
                : base_{base}
                , delta_{delta}
            {
            }

            auto operator*() const -> Value { return base_ + *delta_; }
            auto operator-(ptrdiff_t d) -> const_iterator &
            {
                delta_ -= d;
                return *this;
            }
            auto operator+(ptrdiff_t d) -> const_iterator &
            {
                delta_ + d;
                return *this;
            }
            auto operator--() -> const_iterator &
            {
                --delta_;
                return *this;
            }
            auto operator++() -> const_iterator &
            {
                ++delta_;
                return *this;
            }

            auto operator==(const_iterator const & it) const -> bool
            {
                return base_ == it.base_ and delta_ == it.delta_;
            }

            auto operator!=(const_iterator const & it) const -> bool
            {
                return not (*this == it);
            }
        };

        delta_vector(Base const & base)
            : base_{base}
        {
            // Insert a zero-diff value to compute the first value
            push_back(base);
        }

        auto clear() -> void
        {
            deltas_.clear();
            push_back(base_);
        }

        auto erase(const_iterator const & it) -> const_iterator
        {
            auto delta_it = deltas_.erase(it.delta_);
            return {it.base_, &(*delta_it)};
        }

        auto push_back(Base const & value) -> void
        {
            deltas_.push_back(static_cast<Delta>(value - base_));
        }
        
        auto push_back_delta(Delta const & value) -> void
        {
            deltas_.push_back(value);
        }

        auto size() const -> size_t { return deltas_.size(); }

        auto begin() const -> const_iterator { return {base_, &deltas_[0]}; }
        auto end() const -> const_iterator { return {base_, &deltas_[size()]}; }
    };
}
