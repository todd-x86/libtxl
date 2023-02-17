#pragma once

#include "area.h"
#include "twiddling.h"

#include <vector>
#include <cstddef>

namespace txl 
{
    template<class _Value, size_t _ChunkSize>
    class chunked_vector final
    {
    private:
        using chunk = area<_Value, _ChunkSize>;
        std::vector<chunk> M_chunks_;

        constexpr size_t M_chunk_mask_ = _ChunkSize - 1;
        static_assert(is_power_of_two(_ChunkSize), "chunk size is not a power of two");
    public:
        class iterator final
        {
            
        };

        const_iterator begin() const
        {
        }

        const_iterator end() const
        {
        }

        iterator begin()
        {
        }

        iterator end()
        {
        }

        size_t capacity() const
        {
        }

        void clear()
        {
        }

        bool empty() const
        {
        }

        _Value const & back() const
        {
        }

        _Value const & front() const
        {
        }

        iterator erase(iterator it)
        {
        }

        iterator erase(iterator first, iterator last)
        {
        }

        void insert(iterator it, _Value const & value)
        {
        }

        void push_back(_Value const & value)
        {
        }

        template<class... _Args>
        void emplace_back(_Args && ... args)
        {
        }

        size_t size() const
        {
        }

        void pop_back()
        {
        }

        _Value const & operator[](size_t index) const
        {
        }

        _Value & operator[](size_t index)
        {
        }

        void reserve(size_t capacity)
        {
        }

        void resize(size_t size)
        {
        }
    };
}
