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

    private:
        std::array<unsafe_storage<Value>, Count> data_;
        size_type count_ = 0;

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
            for (size_type i = 0; i < std::min(Count, other.size()); ++i)
            {
                data_[count_] = std::move(other.data_[i]);
                ++count_;
            }

            // Destruct excess values
            for (size_type i = Count; i < other.size(); ++i)
            {
                other.data_[i].erase();
            }

            // Sacrificing stability here by not moving count_ until the end of the move operation
            other.count_ = 0;
        }
    public:
        fixed_vector() = default;
        
        fixed_vector(size_type count)
            : fixed_vector(count, Value{})
        {
        }

        fixed_vector(size_type count, Value const & default_val)
            : count_{std::min(count, Count)}
        {
            assign(count_, default_val);
        }

        template<class Iter, class = std::enable_if<std::is_convertible_v<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag>>>
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

        auto assign(size_type count, Value const & value) -> void
        {
            clear();
            for (size_type i = 0; i < std::min(count, Count); ++i)
            {
                push_back(value);
            }
        }

        template<class Iter, class = std::enable_if<std::is_convertible_v<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag>>>
        auto assign(Iter begin, Iter end) -> void
        {
            clear();
            for (auto const & v : make_iterator_view(begin, end))
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
            return {&(*data_[size()])};
        }

        auto end() const -> const_iterator
        {
            return {&(*data_[size()])};
        }

        auto cend() const noexcept -> const_iterator
        {
            return {&(*data_[size()])};
        }

        auto rbegin() -> reverse_iterator
        {
            return reverse_iterator{end()};
        }

        auto rbegin() const -> const_reverse_iterator
        {
            return const_reverse_iterator{end()};
        }

        auto crbegin() const noexcept -> const_reverse_iterator
        {
            return const_reverse_iterator{cend()};
        }

        auto rend() -> reverse_iterator
        {
            return reverse_iterator{begin()};
        }

        auto rend() const -> const_reverse_iterator
        {
            return const_reverse_iterator{begin()};
        }

        auto crend() const noexcept -> const_reverse_iterator
        {
            return const_reverse_iterator{cbegin()};
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
            for (size_type i = 0; i < count_; ++i)
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

            size_type index = std::distance(cbegin(), pos);
            std::move_backward(pos, cend(), cend() + 1);

            data_[index].emplace(value);
            ++count_;
            return iterator{&(*data_[index])};
        }

        auto insert(const_iterator pos, Value && value) -> result<iterator>
        {
            if (size() == Count)
            {
                return {};
            }

            size_type index = std::distance(cbegin(), pos);
            std::move_backward(&data_[index], &data_[size()], &data_[size()+1]);

            data_[index].emplace(std::move(value));
            ++count_;
            return iterator{&(*data_[index])};
        }

        auto insert(const_iterator pos, size_type count, Value const & value) -> result<iterator>
        {
            if (size()+count > Count)
            {
                return {};
            }

            size_type index = std::distance(cbegin(), pos);
            std::move_backward(&data_[index], &data_[size()], &data_[size()+count]);

            for (size_type i = 0; i < count; ++i)
            {
                data_[index + i].emplace(value);
                ++count_;
            }
            return iterator{&(*data_[index])};
        }

        template<class Iter, class = std::enable_if<std::is_convertible_v<typename std::iterator_traits<Iter>::iterator_category, std::input_iterator_tag>>>
        auto insert(const_iterator pos, Iter begin, Iter end) -> result<iterator>
        {
            size_type count = std::distance(begin, end);
            if (size() + count > Count)
            {
                return {};
            }

            size_type index = std::distance(cbegin(), pos);
            auto orig_index = index;
            std::move_backward(&data_[index], &data_[size()], &data_[size()+count]);

            for (auto const & value : make_iterator_view(begin, end))
            {
                data_[index].emplace(value);
                ++index;
            }
            return iterator{&(*data_[orig_index])};
        }

        auto insert(const_iterator pos, std::initializer_list<Value> list) -> result<iterator>
        {
            if (size() + list.size() > Count)
            {
                return {};
            }

            size_type index = std::distance(cbegin(), pos);
            auto orig_index = index;
            std::move_backward(&data_[index], &data_[size()], &data_[size()+list.size()]);

            for (auto const & value : list)
            {
                data_[index].emplace(value);
                ++index;
            }
            return iterator{&(*data_[orig_index])};
        }

        template<class... Args>
        auto emplace(const_iterator pos, Args && ... args) -> result<iterator>
        {
            if (size() == Count)
            {
                return {};
            }

            size_type index = std::distance(cbegin(), pos);
            std::move_backward(&data_[index], &data_[size()], &data_[size()+1]);

            data_[index].emplace(std::forward<Args>(args)...);
            ++count_;
            return iterator{&(*data_[index])};
        }

        auto erase(iterator pos) -> iterator
        {
            size_type index = std::distance(begin(), pos);
            data_[index].erase();
            std::move_backward(&data_[index+1], &data_[size()], &data_[size()-1]);
            --count_;
            if (index >= count_)
            {
                return end();
            }
            return iterator{&(*data_[index])};
        }

        auto erase(const_iterator pos) -> iterator
        {
            size_type index = std::distance(cbegin(), pos);
            data_[index].erase();
            std::move_backward(&data_[index + 1], &data_[size()], &data_[size()-1]);
            --count_;
            if (index >= count_)
            {
                return end();
            }
            return iterator{&(*data_[index])};
        }

        auto erase(iterator first, iterator last) -> iterator
        {
            return erase(cbegin() + (first - begin()), cbegin() + (last - begin()));
        }

        auto erase(const_iterator first, const_iterator last) -> iterator
        {
            auto old_size = size();
            size_type begin_index = std::distance(cbegin(), first),
                      end_index = std::distance(cbegin(), last);

            for (auto i = begin_index; i < end_index; ++i)
            {
                data_[i].erase();
                --count_;
            }
            std::move_backward(&data_[end_index], &data_[old_size], &data_[old_size-(end_index - begin_index)]);
            if (begin_index >= count_)
            {
                return end();
            }
            return iterator{&(*data_[begin_index])};
        }

        auto push_back(Value const & value) -> result<iterator>
        {
            return emplace_back(value);
        }

        auto push_back(Value && value) -> result<iterator>
        {
            return emplace_back(std::move(value));
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> result<iterator>
        {
            if (count_ == Count)
            {
                return {};
            }

            data_[size()].emplace(std::forward<Args>(args)...);
            ++count_;
            return {iterator{&(*data_[size()-1])}};
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
