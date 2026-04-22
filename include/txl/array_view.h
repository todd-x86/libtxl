#pragma once

// Wrapper for array-like data types

#include <array>
#include <iterator>
#include <type_traits>

namespace txl
{
    /**
     * Represents mutable access to a size-bounded array.
     * 
     * \tparam Value associated array type
     */
    template<class Value>
    class array_view final
    {
    private:
        Value * data_;
        size_t size_;
    public:
        /**
         * Default constructs an empty array_view.
         */
        array_view()
            : array_view(nullptr, nullptr)
        {
        }

        /**
         * Constructs an array_view with beginning and ending boundaries.
         *
         * \param begin lower boundary
         * \param end upper boundary
         */
        array_view(Value * begin, Value * end)
            : data_(begin)
            , size_(std::distance(begin, end))
        {
        }
        
        /**
         * Constructs an array_view of one element.
         *
         * \param value associated value
         */
        array_view(Value & value)
            : array_view(&value, std::next(&value, 1))
        {
        }
    
        /**
         * Utility constructor which creates an array_view on a std::array<>.
         *
         * \param data std::array<> as input
         */
        template<size_t Size>
        array_view(std::array<Value, Size> & data)
            : array_view(&data[0], &data[Size])
        {
        }

        /**
         * Utility constructor which creates an array_view on a fixed-size array.
         *
         * \param values fixed-size array as input
         */
        template<size_t Size>
        array_view(Value (&values)[Size])
            : array_view(&values[0], &values[Size])
        {
        }
        
        /**
         * Const-reference-based element access to an array_view's contents.
         *
         * \param index zero-based index into array_view's contents
         * 
         * \return underlying value by const-reference
         */
        auto operator[](size_t index) const -> Value const & { return *std::next(data_, index); }
        
        /**
         * Reference-based element access to an array_view's contents.
         *
         * \param index zero-based index into array_view's contents
         * 
         * \return underlying value by reference
         */
        auto operator[](size_t index) -> Value & { return *std::next(data_, index); }
        
        /**
         * Pointer to beginning of array_view's underlying buffer.
         *
         * \return pointer to beginning of array
         */
        auto begin() -> Value * { return data_; }
        
        /**
         * Pointer to end of array_view's underlying buffer.
         *
         * \return pointer to end of array
         */
        auto end() -> Value * { return std::next(data_, size_); }
        
        /**
         * Const pointer to beginning of array_view's underlying buffer.
         *
         * \return const pointer to beginning of array
         */
        auto begin() const -> Value const * { return data_; }
        
        /**
         * Const pointer to end of array_view's underlying buffer.
         *
         * \return const pointer to end of array
         */
        auto end() const -> Value const * { return std::next(data_, size_); }
        
        /**
         * Const pointer to beginning of array_view's underlying buffer.
         *
         * \return const pointer to beginning of array
         */
        auto cbegin() const -> Value const * { return data_; }
        
        /**
         * Const pointer to end of array_view's underlying buffer.
         *
         * \return const pointer to end of array
         */
        auto cend() const -> Value const * { return std::next(data_, size_); }

        /**
         * Const pointer to array_view's underlying buffer.
         * 
         * \return const pointer to beginning of underlying buffer
         */
        auto data() const -> Value const * { return data_; }
        
        /**
         * Pointer to array_view's underlying buffer.
         *
         * \return pointer to beginning of underlying buffer
         */
        auto data() -> Value * { return data_; }
        
        /**
         * Determines if the array_view is empty (i.e. size == 0).
         *
         * \return true if the size of the array_view is 0
         */
        auto empty() const -> bool { return size() == 0; }

        /**
         * Returns the size of the underlying buffer in bytes.
         *
         * \return size of array_view in bytes
         */
        auto size() const -> size_t { return size_; }
        
        auto slice(size_t pos, size_t len) const -> array_view<Value>
        {
            pos = std::min(pos, size_);
            len = std::min(len, size_ - pos);
            return {std::next(data_, pos), std::next(data_, pos + len)};
        }
        
        auto slice(size_t pos) const -> array_view<Value>
        {
            pos = std::min(pos, size_);
            return {std::next(data_, pos), std::next(data_, size_)};
        }
    };
}
