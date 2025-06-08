#pragma once

#include <txl/iterators.h>

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
        {
        }

        fixed_vector(size_t, Value const & default_val)
        {
        }

        template<class Iter>
        fixed_vector(Iter begin, Iter end)
        {
        }

        fixed_vector(fixed_vector const & other)
        {
        }

        fixed_vector(fixed_vector && other)
        {
        }

        fixed_vector(std::initializer_list<Value> list)
        {
        }

        ~fixed_vector()
        {
        }

        auto operator=(fixed_vector const & other) -> fixed_vector &
        {
        }

        auto operator=(fixed_vector && other) -> fixed_vector &
        {
        }

        auto assign(size_t count, Value const & value) -> void
        {
        }

        template<class Iter>
        auto assign(Iter begin, Iter end) -> void
        {
        }

        auto assign(std::initializer_list<Value> list) -> void
        {
        }

        auto at(size_type pos) -> reference
        {
        }

        auto at(size_type pos) const -> const_reference
        {
        }

        auto operator[](size_type pos) -> reference
        {
        }
        
        auto operator[](size_type pos) const -> const_reference
        {
        }

        auto front() -> reference
        {
        }

        auto front() const -> const_reference
        {
        }

        auto back() -> reference
        {
        }

        auto back() const -> const_reference
        {
        }

        auto data() -> Value *
        {
        }

        auto data() const -> Value const *
        {
        }

        auto begin() -> iterator
        {
        }

        auto begin() const -> const_iterator
        {
        }

        auto cbegin() const noexcept -> const_iterator
        {
        }

        auto end() -> iterator
        {
        }

        auto end() const -> const_iterator
        {
        }

        auto cend() const noexcept -> const_iterator
        {
        }

        auto rbegin() -> reverse_iterator
        {
        }

        auto rbegin() const -> const_reverse_iterator
        {
        }

        auto crbegin() const noexcept -> const_reverse_iterator
        {
        }

        auto rend() -> reverse_iterator
        {
        }

        auto rend() const -> const_reverse_iterator
        {
        }

        auto crend() const noexcept -> const_reverse_iterator
        {
        }

        auto empty() const -> bool
        {
        }

        auto size() const -> size_type
        {
        }

        auto max_size() const -> size_type
        {
        }

        auto capacity() const -> size_type
        {
        }

        auto clear() -> void
        {
        }

        auto insert(const_iterator pos, Value const & value) -> iterator
        {
        }

        auto insert(const_iterator pos, Value && value) -> iterator
        {
        }

        auto insert(const_iterator pos, size_type count, Value const & value) -> iterator
        {
        }

        template<class Iter>
        auto insert(const_iterator pos, Iter begin, Iter end) -> iterator
        {
        }

        auto insert(const_iterator pos, std::initializer_list<Value> list) -> iterator
        {
        }

        template<class... Args>
        auto emplace(const_iterator pos, Args && ... args) -> iterator
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

        auto push_back(Value const & value) -> void
        {
        }

        auto push_back(Value && value) -> void
        {
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> reference
        {
        }

        auto pop_back() -> void
        {
        }

        template<size_t S>
        auto swap(fixed_vector<Value, S> & other) -> void
        {
        }

        template<size_t S>
        auto operator==(fixed_vector<Value, S> const & other) const -> bool
        {
        }
        
        template<size_t S>
        auto operator!=(fixed_vector<Value, S> const & other) const -> bool
        {
        }
        
        template<size_t S>
        auto operator<(fixed_vector<Value, S> const & other) const -> bool
        {
        }
        
        template<size_t S>
        auto operator<=(fixed_vector<Value, S> const & other) const -> bool
        {
        }
        
        template<size_t S>
        auto operator>(fixed_vector<Value, S> const & other) const -> bool
        {
        }
        
        template<size_t S>
        auto operator>(fixed_vector<Value, S> const & other) const -> bool
        {
        }
    };
}
