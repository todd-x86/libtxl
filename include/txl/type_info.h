#pragma once

#include <cstring>
#include <ostream>
#include <string>

namespace txl
{
    // Compile-time type information
    struct type_info final
    {
        char const * const name;
        const size_t name_len;
        const size_t hash;

        inline bool operator==(type_info const & ti) const
        {
            // Compare name pointers before string contents
            // (names point to statically-allocated __PRETTY_FUNCTION__ macros)
            return hash == ti.hash &&
                   name_len == ti.name_len &&
                   (name == ti.name || strncmp(name, ti.name, name_len) == 0);
        }

        std::string str() const { return std::string(name, name_len); }

        inline bool operator!=(type_info const & ti) const
        {
            return !(*this == ti);
        }
    };

    inline std::ostream & operator<<(std::ostream & os, type_info const & ti)
    {
        os << "type_info { name=\"";
        auto n = ti.name;
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
        inline size_t operator()(txl::type_info const & ti) const
        {
            return ti.hash;
        }
    };
}

