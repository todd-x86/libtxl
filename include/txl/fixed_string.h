#pragma once

#include <cstdlib>
#include <cstring>
#include <algorithm>

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

        size_t capacity() const
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
            auto bytes_to_copy = std::min(str.size(), this->capacity()-1);
            std::copy(str.data(), std::next(str.data(), bytes_to_copy), this->data());
            if (bytes_to_copy < this->capacity()-1)
            {
                (*this)[bytes_to_copy] = '\0';
            }
        }

        auto to_string_view() const -> std::string_view
        {
            return {this->data(), size()};
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
        
        auto size() const -> size_t
        {
            // If there's a null-terminator, call strlen() otherwise return capacity()
            if ((*this)[this->capacity()-1] == static_cast<Char>(0))
            {
                return std::strlen(this->data());
            }

            return this->capacity();
        }
    };
}
