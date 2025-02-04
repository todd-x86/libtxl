#pragma once

#include <txl/types.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <iterator>
#include <array>
#include <cstring>

namespace txl
{
    class buffer_ref final
    {
    private:
        void * buffer_ = nullptr;
        size_t length_ = 0;
    public:
        buffer_ref() = default;

        template<class Char>
        buffer_ref(std::basic_string_view<Char> s)
            : buffer_ref(static_cast<void *>(const_cast<char *>(s.data())), s.size())
        {
        }
        
        template<class Char>
        buffer_ref(std::basic_string<Char> const & s)
            : buffer_ref(static_cast<void *>(const_cast<char *>(s.data())), s.size())
        {
        }

        template<class Char, size_t Size>
        buffer_ref(std::array<Char, Size> & buf)
            : buffer_ref(buf.data(), Size)
        {
        }

        template<size_t Size>
        buffer_ref(char (&s)[Size])
            : buffer_ref(static_cast<void *>(&s[0]), Size)
        {
        }
        
        template<size_t Size>
        buffer_ref(char const (&s)[Size])
            : buffer_ref(static_cast<void *>(const_cast<char *>(&s[0])), Size)
        {
        }
        
        template<size_t Size>
        buffer_ref(std::byte (&b)[Size])
            : buffer_ref(static_cast<void *>(&b[0]), Size)
        {
        }
        
        buffer_ref(std::byte * buffer, size_t length)
            : buffer_ref(static_cast<void *>(buffer), length)
        {
        }
        
        buffer_ref(std::byte const * buffer, size_t length)
            : buffer_ref(const_cast<std::byte *>(buffer), length)
        {
        }

        buffer_ref(byte_vector & b)
            : buffer_ref(static_cast<void *>(b.data()), b.size())
        {
        }
        
        buffer_ref(byte_vector const & b)
            : buffer_ref(static_cast<void *>(const_cast<byte_vector &>(b).data()), b.size())
        {
        }
        
        buffer_ref(void const * buffer, size_t length)
            : buffer_ref(const_cast<void *>(buffer), length)
        {
        }

        buffer_ref(void * buffer, size_t length)
            : buffer_(buffer)
            , length_(length)
        {
        }

        template<class T>
        static inline auto cast(T const & data) -> buffer_ref
        {
            return {reinterpret_cast<void *>(const_cast<T *>(&data)), sizeof(T)};
        }
        
        template<class T>
        static inline auto cast(T & data) -> buffer_ref
        {
            return {reinterpret_cast<void *>(&data), sizeof(T)};
        }

        auto compare(buffer_ref const & other) const -> int
        {
            auto bytes_to_compare = std::min(size(), other.size());
            auto cmp = std::memcmp(data(), other.data(), bytes_to_compare);
            if (cmp != 0)
            {
                return cmp;
            }

            // Compare sizes
            if (size() > other.size())
            {
                return 1;
            }
            if (size() < other.size())
            {
                return -1;
            }
            return 0;
        }

        auto equal(buffer_ref const & other) const -> bool
        {
            return compare(other) == 0;
        }

        auto operator==(buffer_ref const & other) const -> bool
        {
            return data() == other.data() and size() == other.size();
        }
        auto operator!=(buffer_ref const & other) const -> bool
        {
            return not (*this == other);
        }

        auto slice_n(size_t pos, size_t len) -> buffer_ref
        {
            return slice(pos, pos + len);
        }

        auto slice(size_t begin, size_t end) -> buffer_ref
        {
            auto buf = static_cast<char *>(buffer_);
            if (begin >= length_ or end <= begin)
            {
                return {};
            }
            end = std::min(length_ - begin, end - begin);
            return {static_cast<void *>(std::next(buf, begin)), end};
        }

        auto slice(size_t begin) -> buffer_ref
        {
            return slice(begin, length_);
        }

        auto copy_from(buffer_ref b) -> size_t
        {
            auto bytes_to_copy = std::min(b.size(), size());
            auto src_data = b.begin();
            std::copy(src_data, std::next(src_data, bytes_to_copy), begin()); 
            return bytes_to_copy;
        }

        auto operator=(std::string_view s) -> buffer_ref
        {
            // TODO: is this bad design?
            copy_from(s);
            return *this;
        }
        
        auto operator[](size_t index) -> std::byte &
        {
            return *std::next(begin(), index);
        }
        
        auto operator[](size_t index) const -> std::byte const &
        {
            return *std::next(begin(), index);
        }

        auto to_string_view() const -> std::string_view { return {static_cast<char const *>(data()), size()}; }
        
        template<class T>
        auto to_alias(size_t byte_offset = 0) -> T * { return reinterpret_cast<T *>(std::next(begin(), byte_offset)); }
        
        template<class T>
        auto to_alias(size_t byte_offset = 0) const -> T const * { return reinterpret_cast<T const *>(std::next(begin(), byte_offset)); }

        auto empty() const -> bool { return length_ == 0; }
        auto data() -> void * { return buffer_; }
        auto data() const -> void const * { return buffer_; }
        auto size() const -> size_t { return length_; }
        auto begin() -> std::byte * { return static_cast<std::byte *>(buffer_); }
        auto end() -> std::byte * { return std::next(begin(), length_); }
        auto begin() const -> std::byte const * { return static_cast<std::byte const *>(buffer_); }
        auto end() const -> std::byte const * { return std::next(begin(), length_); }
    };
}
