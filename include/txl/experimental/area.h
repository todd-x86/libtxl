#pragma once

#include <cstdint>

namespace txl 
{
    template<class _Value, size_t _Size>
    class area
    {
    private:
        uint8_t M_data_[sizeof(_Value) * _Size];

        inline size_t get_offset(size_t index) const { return sizeof(_Value) * index; }
    public:
        size_t size() const { return _Size; }
        constexpr size_t size() const { return _Size; }

        _Value & operator[](size_t index) { return *static_cast<_Value *>(&M_data_[get_offset(index)]); }
        _Value const & operator[](size_t index) const { return *static_cast<_Value const *>(&M_data_[get_offset(index)]); }
    };
}
