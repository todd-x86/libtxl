#pragma once

#include <txl/iterators.h>
#include <txl/iterator_view.h>
#include <txl/storage.h>
#include <txl/result.h>

#include <algorithm>
#include <array>
#include <iterator>

namespace txl
{
    template<class Value, size_t Count>
    class fixed_vector
    {
    private:
        storage<Value> data_[Count];
        size_t count_ = 0;

        template<size_t S>
        auto copy(fixed_vector<Value, S> const & other) -> void
        {
            for (auto const & v : other)
            {
                push_back(v);
            }
        }

        template<size_t S>
        auto move(fixed_vector<Value, S> && other) -> void
        {
            for (size_t i = 0; i < std::min(Count, other.size()); ++i)
            {
                data_[count_] = std::move(other.data_[i]);
                ++count_;
            }

            // Destruct excess values
            for (size_t i = Count; i < other.size(); ++i)
            {
                other.data_[i].erase();
            }
        }
    public:
        using value_type = Value;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = Value &;
        using const_reference = Value const &;
        using pointer = Value *;
        using const_pointer = Value const *;
        using iterator = basic_iterator<Value>;
        using const_iterator = basic_iterator<Value const>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        fixed_vector() = default;
        
        fixed_vector(size_t count)
            : fixed_vector(count, Value{})
        {
        }

        fixed_vector(size_t count, Value const & default_val)
            : count_{std::min(count, Count)}
        {
            assign(count_, default_val);
        }

        template<class Iter>
        fixed_vector(Iter begin, Iter end)
        {
            assign(begin, end);
        }

        template<size_t S>
        fixed_vector(fixed_vector<Value, S> const & other)
        {
            copy(other);
        }

        template<size_t S>
        fixed_vector(fixed_vector<Value, S> && other)
        {
            move(std::move(other));
        }

        fixed_vector(std::initializer_list<Value> list)
        {
            assign(list);
        }

        ~fixed_vector()
        {
            clear();
        }

        template<size_t S>
        auto operator=(fixed_vector<Value, S> const & other) -> fixed_vector &
        {
            if (&other != this)
            {
                clear();
                copy(other);
            }

            return *this;
        }

        template<size_t S>
        auto operator=(fixed_vector<Value, S> && other) -> fixed_vector &
        {
            if (&other != this)
            {
                clear();
                move(std::move(other));
            }

            return *this;
        }

        auto assign(size_t count, Value const & value) -> void
        {
            clear();
            for (size_t i = 0; i < std::min(count, Count); ++i)
            {
                push_back(value);
            }
        }

        template<class Iter>
        auto assign(Iter begin, Iter end) -> void
        {
            clear();
            for (auto const & v : iterator_view{begin, end})
            {
                push_back(v);
            }
        }

        auto assign(std::initializer_list<Value> list) -> void
        {
            clear();
            for (auto const & v : list)
            {
                push_back(v);
            }
        }

        auto at(size_type pos) -> reference
        {
            if (pos < 0 or pos >= size())
            {
                throw std::out_of_range{"Element index is out of range"};
            }
            return *data_[pos];
        }

        auto at(size_type pos) const -> const_reference
        {
            if (pos < 0 or pos >= size())
            {
                throw std::out_of_range{"Element index is out of range"};
            }
            return *data_[pos];
        }

        auto operator[](size_type pos) -> reference
        {
            return *data_[pos];
        }
        
        auto operator[](size_type pos) const -> const_reference
        {
            return *data_[pos];
        }

        auto front() -> reference
        {
            return *data_[0];
        }

        auto front() const -> const_reference
        {
            return *data_[0];
        }

        auto back() -> reference
        {
            return *data_[size()-1];
        }

        auto back() const -> const_reference
        {
            return *data_[size()-1];
        }

        auto data() -> Value *
        {
            return &(*data_[0]);
        }

        auto data() const -> Value const *
        {
            return &(*data_[0]);
        }

        auto begin() -> iterator
        {
            return {&(*data_[0])};
        }

        auto begin() const -> const_iterator
        {
            return {&(*data_[0])};
        }

        auto cbegin() const noexcept -> const_iterator
        {
            return {&(*data_[0])};
        }

        auto end() -> iterator
        {
            return {&(*data_[size()-1])};
        }

        auto end() const -> const_iterator
        {
            return {&(*data_[size()-1])};
        }

        auto cend() const noexcept -> const_iterator
        {
            return {&(*data_[size()-1])};
        }

        auto rbegin() -> reverse_iterator
        {
            return {basic_iterator<Value>{&(*data_[size()-1])}};
        }

        auto rbegin() const -> const_reverse_iterator
        {
            return {basic_iterator<Value>{&(*data_[size()-1])}};
        }

        auto crbegin() const noexcept -> const_reverse_iterator
        {
            return {basic_iterator<Value>{&(*data_[size()-1])}};
        }

        auto rend() -> reverse_iterator
        {
            return {basic_iterator<Value>{&(*data_[0])}};
        }

        auto rend() const -> const_reverse_iterator
        {
            return {basic_iterator<Value>{&(*data_[0])}};
        }

        auto crend() const noexcept -> const_reverse_iterator
        {
            return {basic_iterator<Value>{&(*data_[0])}};
        }

        auto empty() const -> bool
        {
            return size() == 0;
        }

        auto size() const -> size_type
        {
            return count_;
        }

        auto max_size() const -> size_type
        {
            return Count;
        }

        auto capacity() const -> size_type
        {
            return Count;
        }

        auto clear() -> void
        {
            for (size_t i = 0; i < count_; ++i)
            {
                data_[i].erase();
            }
            count_ = 0;
        }

        auto insert(const_iterator pos, Value const & value) -> result<iterator>
        {
            if (size() == Count)
            {
                return {};
            }

            auto index = std::distance(begin(), pos);

            // Insert at the end and move it one-by-one into the correct position
            data[count_].emplace(value);
            for (auto i = count_; i > index; --i)
            {
                data_[i-1].swap(data_[i]);
            }
            return iterator{&(*data_[index])};
        }

        auto insert(const_iterator pos, Value && value) -> result<iterator>
        {
            if (size() == Count)
            {
                return {};
            }

            auto index = std::distance(begin(), pos);

            // Insert at the end and move it one-by-one into the correct position
            data[count_].emplace(std::move(value));
            for (auto i = count_; i > index; --i)
            {
                data_[i-1].swap(data_[i]);
            }
            return iterator{&(*data_[index])};
        }

        auto insert(const_iterator pos, size_type count, Value const & value) -> result<iterator>
        {
        }

        template<class Iter>
        auto insert(const_iterator pos, Iter begin, Iter end) -> result<iterator>
        {
        }

        auto insert(const_iterator pos, std::initializer_list<Value> list) -> result<iterator>
        {
        }

        template<class... Args>
        auto emplace(const_iterator pos, Args && ... args) -> result<iterator>
        {
        }

        auto erase(iterator pos) -> iterator
        {
        }

        auto erase(const_iterator pos) -> iterator
        {
        }

        auto erase(iterator first, iterator last) -> iterator
        {
        }

        auto erase(const_iterator first, const_iterator last) -> iterator
        {
        }

        auto push_back(Value const & value) -> result<iterator>
        {
            if (count_ == Count)
            {
                return false;
            }

            data_[size()].emplace(value);
            ++count_;
            return true;
        }

        auto push_back(Value && value) -> result<iterator>
        {
            if (count_ == Count)
            {
                return false;
            }

            data_[size()].emplace(std::move(value));
            ++count_;
            return true;
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> result<iterator>
        {
            if (count_ == Count)
            {
                return false;
            }

            data_[size()].emplace(std::forward<Args>(args)...);
            ++count_;
            return true;
        }

        auto pop_back() -> void
        {
            data_[size()-1].erase();
            --count_;
        }

        auto swap(fixed_vector & other) -> void
        {
            std::swap(other.data_, data_);
            std::swap(other.count_, count_);
        }

        template<size_t S>
        auto operator==(fixed_vector<Value, S> const & other) const -> bool
        {
            return size() == other.size() and
                   std::equal(begin(), end(), other.begin());
        }
        
        template<size_t S>
        auto operator!=(fixed_vector<Value, S> const & other) const -> bool
        {
            return not (*this == other);
        }
        
        template<size_t S>
        auto operator<(fixed_vector<Value, S> const & other) const -> bool
        {
            return std::lexicographical_compare(begin(), end(), other.begin(), other.end());
        }
        
        template<size_t S>
        auto operator<=(fixed_vector<Value, S> const & other) const -> bool
        {
            return not (other < *this);
        }
        
        template<size_t S>
        auto operator>(fixed_vector<Value, S> const & other) const -> bool
        {
            return other < *this;
        }
        
        template<size_t S>
        auto operator>=(fixed_vector<Value, S> const & other) const -> bool
        {
            return not (*this < other);
        }
    };
}
