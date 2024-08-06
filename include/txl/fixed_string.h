#pragma once

#include <cstdlib>
#include <cstring>
#include <algorithm>
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

        Char & operator[](size_t index)
        {
            return data_[index];
        }

        Char const & operator[](size_t index) const
        {
            return data_[index];
        }
    };

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

        auto clear()
        {
            (*this)[0] = '\0';
        }

        auto copy_from(char const * str)
        {
            copy_from(std::string_view{str});
        }
        
        auto copy_from(std::string const & str)
        {
            copy_from(std::string_view{str});
        }

        template<size_t OtherSize>
        auto copy_from(fixed_string<OtherSize, Char> const & str)
        {
            copy_from(str.to_string_view());
        }
        
        auto copy_from(std::string_view str)
        {
            auto bytes_to_copy = std::min(str.size(), capacity());
            std::copy(str.data(), std::next(str.data(), bytes_to_copy), this->data());
            (*this)[bytes_to_copy] = '\0';
        }

        auto to_string_view() const -> std::string_view
        {
            return {this->data(), size()};
        }

        auto capacity() const -> size_t
        {
            return Size;
        }

        auto size() const -> size_t
        {
            return std::strlen(this->data());
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

        template<size_t OtherSize>
        auto operator==(fixed_string<OtherSize, Char> const & str) const -> bool
        {
            return to_string_view() == str.to_string_view();
        }
        
        template<size_t OtherSize>
        auto operator!=(fixed_string<OtherSize, Char> const & str) const -> bool
        {
            return not (*this == str);
        }

        auto operator==(char const * str) const -> bool
        {
            return std::string_view{str} == to_string_view();
        }
        
        auto operator!=(char const * str) const -> bool
        {
            return not (*this == str);
        }
        
        auto operator==(std::string_view str) const -> bool
        {
            return str == to_string_view();
        }
        
        auto operator!=(std::string_view str) const -> bool
        {
            return not (*this == str);
        }
    };


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

        auto clear()
        {
            (*this)[0] = '\0';
        }

        auto copy_from(char const * str)
        {
            copy_from(std::string_view{str});
        }
        
        auto copy_from(std::string const & str)
        {
            copy_from(std::string_view{str});
        }

        template<size_t OtherSize>
        auto copy_from(string_block<OtherSize, Char> const & str)
        {
            copy_from(str.to_string_view());
        }
        
        auto copy_from(std::string_view str)
        {
            auto bytes_to_copy = std::min(str.size(), capacity());
            std::copy(str.data(), std::next(str.data(), bytes_to_copy), this->data());
            if (bytes_to_copy < capacity())
            {
                (*this)[bytes_to_copy] = '\0';
            }
        }

        auto to_string_view() const -> std::string_view
        {
            return {this->data(), size()};
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

        template<size_t OtherSize>
        auto operator==(string_block<OtherSize, Char> const & str) const -> bool
        {
            return to_string_view() == str.to_string_view();
        }
        
        template<size_t OtherSize>
        auto operator!=(string_block<OtherSize, Char> const & str) const -> bool
        {
            return not (*this == str);
        }

        auto operator==(char const * str) const -> bool
        {
            return std::string_view{str} == to_string_view();
        }
        
        auto operator!=(char const * str) const -> bool
        {
            return not (*this == str);
        }
        
        auto operator==(std::string_view str) const -> bool
        {
            return str == to_string_view();
        }
        
        auto operator!=(std::string_view str) const -> bool
        {
            return not (*this == str);
        }
        
        auto size() const -> size_t
        {
            // If there's a null-terminator, call strlen() otherwise return capacity()
            if ((*this)[capacity()-1] == static_cast<Char>(0))
            {
                return std::strlen(this->data());
            }

            return capacity();
        }

        auto capacity() const -> size_t
        {
            return Size;
        }
    };
}
