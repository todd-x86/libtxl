#pragma once

namespace txl
{

template<size_t Size, class Char = char>
class fixed_string_base
{
private:
    Char data_[Size];
public:
    using iterator = Char *;
    using const_iterator = Char const *;

    auto data() const -> Char { return data_; }

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

    auto size() const -> size_t
    {
        return std::strlen(data());
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
        if ((*this)[capacity()-1] == 0)
        {
            return std::strlen(data());
        }

        return capacity();
    }
};

}
