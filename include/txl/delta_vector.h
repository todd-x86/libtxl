#pragma once

#include <vector>

namespace txl
{
    template<class Base, class Delta, class Value = decltype(std::declval<Base>() + std::declval<Delta>())>
    class delta_vector
    {
    private:
        Base base_;
        Base last_;
        std::vector<Delta> deltas_;
    public:
        using value_type = Value;
        
        class const_iterator final
        {
        private:
            Base last_;
            Delta const * delta_;
        public:
            const_iterator(Base const & base, Delta const * delta)
                : last_{base}
                , delta_{delta}
            {
            }

            auto operator*() const -> Value { return last_ + *delta_; }
            auto operator--() -> const_iterator &
            {
                last_ -= *delta_;
                --delta_;
                return *this;
            }
            auto operator++() -> const_iterator &
            {
                last_ += *delta_;
                ++delta_;
                return *this;
            }

            auto operator==(const_iterator const & it) const -> bool
            {
                return delta_ == it.delta_;
            }

            auto operator!=(const_iterator const & it) const -> bool
            {
                return not (*this == it);
            }
        };

        delta_vector(Base const & base)
            : base_{base}
            , last_{base}
        {
            // Insert a zero-diff value to compute the first value
            push_back_delta(base - base);
        }

        auto clear() -> void
        {
            deltas_.clear();
            last_ = base_;
            push_back_delta(base_ - base_);
        }

        auto push_back(Base const & value) -> void
        {
            push_back_delta(static_cast<Delta>(value - last_));
        }
        
        auto push_back_delta(Delta const & value) -> void
        {
            last_ += value;
            deltas_.push_back(value);
        }

        auto size() const -> size_t { return deltas_.size(); }

        auto begin() const -> const_iterator { return {base_, &deltas_[0]}; }
        auto end() const -> const_iterator { return {base_, &deltas_[size()]}; }
    };
}
