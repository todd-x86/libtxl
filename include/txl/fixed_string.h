#pragma once

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace txl
{
    template<size_t Size, class Char = char>
    class fixed_string_base
    {
    private:
        Char data_[Size];
    protected:
        auto data() -> Char * { return data_; }
    public:
        using iterator = Char *;
        using const_iterator = Char const *;

        auto data() const -> Char const * { return data_; }

        iterator begin()
        {
            return std::begin(data_);
        }

        iterator end()
        {
            return std::end(data_);
        }

        const_iterator begin() const
        {
            return std::begin(data_);
        }

        const_iterator end() const
        {
            return std::end(data_);
        }

        size_t block_size() const
        {
            return Size;
        }
        
        auto size() const -> size_t
        {
            // Find distance to null terminator, otherwise just return max capacity
            auto it = std::find(begin(), end(), static_cast<Char>(0));
            return std::distance(begin(), it);
        }

        auto to_string_view() const -> std::basic_string_view<Char>
        {
            return {data(), size()};
        }
        
        auto clear()
        {
            (*this)[0] = static_cast<Char>(0);
        }

        Char & operator[](size_t index)
        {
            return data_[index];
        }

        Char const & operator[](size_t index) const
        {
            return data_[index];
        }

        template<size_t OtherSize>
        auto operator==(fixed_string_base<OtherSize, Char> const & str) const -> bool
        {
            return to_string_view() == str.to_string_view();
        }
        
        template<size_t OtherSize>
        auto operator!=(fixed_string_base<OtherSize, Char> const & str) const -> bool
        {
            return not (*this == str);
        }

        auto operator==(std::basic_string_view<Char> str) const -> bool
        {
            return str == to_string_view();
        }
        
        auto operator!=(std::basic_string_view<Char> str) const -> bool
        {
            return not (*this == str);
        }
    };

    /**
     * Fixed string data structure which encapsulates a series of characters plus a null-terminator in the same memory allocation as the container.
     * The main purpose of a `fixed_string` is to prevent heap allocations of strings that are subject to change frequently but obey a specific length boundary.
     */
    template<size_t Size, class Char = char>
    class fixed_string : public fixed_string_base<Size + 1, Char>
    {
    public:
        using fixed_string_base<Size + 1, Char>::fixed_string_base;

        template<class String>
        fixed_string(String && str)
            : fixed_string()
        {
            copy_from(str);
        }

        fixed_string(fixed_string && data)
            : fixed_string(data)
        {
            // Emulate a swap
            data.clear();
        }
        
        auto copy_from(Char const * str)
        {
            copy_from(std::basic_string_view<Char>{str});
        }

        auto copy_from(std::basic_string<Char> const & str)
        {
            copy_from(std::basic_string_view<Char>{str});
        }

        template<size_t OtherSize>
        auto copy_from(fixed_string<OtherSize, Char> const & str)
        {
            copy_from(str.to_string_view());
        }
        
        auto copy_from(std::basic_string_view<Char> str)
        {
            auto bytes_to_copy = std::min(str.size(), capacity());
            std::copy(str.data(), std::next(str.data(), bytes_to_copy), this->data());
            (*this)[bytes_to_copy] = static_cast<Char>(0);
        }

        auto capacity() const -> size_t
        {
            return Size;
        }

        template<class String>
        auto operator=(String const & str) -> fixed_string &
        {
            copy_from(str);
            return *this;
        }
        
        template<class String>
        auto operator=(String && str) -> fixed_string &
        {
            copy_from(str);
            if constexpr (std::is_same_v<String, fixed_string>)
            {
                if (&str != this)
                {
                    // Emulate a swap
                    str.clear();
                }
            }
            return *this;
        }
    };


    /**
     * Fixed length string data structure that does not include a null terminator. The key difference between `string_block` and `fixed_string` is a `fixed_string` supports a null-terminator which permits it to be used in C-string ABIs, whereas a `string_block` may not always guarantee that your string includes a null terminator.
     */
    template<size_t Size, class Char = char>
    class string_block : public fixed_string_base<Size, Char>
    {
    public:
        // Use all block
        using fixed_string_base<Size, Char>::fixed_string_base;

        template<class String>
        string_block(String && str)
            : string_block()
        {
            copy_from(str);
        }

        string_block(string_block && data)
            : string_block(data)
        {
            // Emulate a swap
            data.clear();
        }

        auto copy_from(Char const * str)
        {
            copy_from(std::basic_string_view<Char>{str});
        }

        auto copy_from(std::basic_string<Char> const & str)
        {
            copy_from(std::basic_string_view<Char>{str});
        }

        template<size_t OtherSize>
        auto copy_from(string_block<OtherSize, Char> const & str)
        {
            copy_from(str.to_string_view());
        }
        
        auto copy_from(std::basic_string_view<Char> str)
        {
            auto bytes_to_copy = std::min(str.size(), capacity());
            std::copy(str.data(), std::next(str.data(), bytes_to_copy), this->data());
            if (bytes_to_copy < capacity())
            {
                (*this)[bytes_to_copy] = static_cast<Char>(0);
            }
        }

        template<class String>
        auto operator=(String const & str) -> string_block &
        {
            copy_from(str);
            return *this;
        }
        
        template<class String>
        auto operator=(String && str) -> string_block &
        {
            copy_from(str);
            if constexpr (std::is_same_v<String, string_block>)
            {
                if (&str != this)
                {
                    // Emulate a swap
                    str.clear();
                }
            }
            return *this;
        }

        auto capacity() const -> size_t
        {
            return Size;
        }
    };
}
