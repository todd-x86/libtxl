#pragma once

#include <cstring>
#include <ostream>
#include <string>
#include <string_view>

namespace txl
{
    // Compile-time type information
    struct type_info final
    {
        char const * name_data = nullptr;
        size_t name_len = 0;
        size_t hash = 0;

        auto empty() const -> bool
        {
            return name_len == 0 and hash == 0;
        }

        auto operator==(type_info const & ti) const -> bool
        {
            // Compare name pointers before string contents
            // (names point to statically-allocated __PRETTY_FUNCTION__ macros)
            return hash == ti.hash &&
                   name_len == ti.name_len &&
                   (name_data == ti.name_data || strncmp(name_data, ti.name_data, name_len) == 0);
        }

        auto name() const -> std::string_view { return {name_data, name_len}; }

        auto str() const -> std::string { return std::string(name_data, name_len); }

        auto operator!=(type_info const & ti) const -> bool
        {
            return not (*this == ti);
        }
    };

    inline auto operator<<(std::ostream & os, type_info const & ti) -> std::ostream &
    {
        os << "type_info { name=\"";
        auto n = ti.name_data;
        for (size_t i = 0; i != ti.name_len; ++i)
        {
            os.put(*n);
            ++n;
        }
        os << "\", hash=" << ti.hash << " }";
        return os;
    }

    // Extracts type info at compile-time
    template<class E>
    constexpr type_info get_type_info()
    {
        // Dissect __PRETTY_FUNCTION__
        // example: "constexpr type_info get_type_info() [with E = price_update]"
        char const * fn = __PRETTY_FUNCTION__;

        // DJB2 hash setup
        size_t l = 0;
        size_t hash = 5381;
        char const * p = fn;

        // Find '='
        while (*p != '\0' && *p != '=')
        {
            ++p;
        }
        if (*p == '=')
        {
            ++p;
            while (*p == ' ')
            {
                ++p;
            }
        }

        // Capture portion of type name and hash
        char const * type_name = p;
        while (*p != '\0' && *p != ']')
        {
            hash = ((hash >> 5) + hash) + *p;
            ++p;
            ++l;
        }
        return type_info{ type_name, l, hash };
    }
    
    // Return hash for compile-time type info
    template<class E>
    constexpr size_t get_type_hash()
    {
        return get_type_info<E>().hash;
    }
}

// Hash specialization
namespace std
{
    template<>
    struct hash<txl::type_info>
    {
        auto operator()(txl::type_info const & ti) const -> size_t
        {
            return ti.hash;
        }
    };
}

