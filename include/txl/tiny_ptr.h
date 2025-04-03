#pragma once

#include <cstdint>
#include <iterator>
#include <type_traits>

namespace txl
{
    class tiny_ptr_base
    {
        template<class, class, size_t>
        friend class tiny_ptr;
    private:
        std::byte const * base_ = nullptr;

        template<class T>
        auto init_once(T const * ptr) -> void
        {
            if (base_ == nullptr)
            {
                // We need to subtract a small value from the base such that the first value is not interpreted as null
                base_ = reinterpret_cast<std::byte const *>(ptr) - sizeof(void *);
            }
        }
    public:
        tiny_ptr_base() = default;

        template<class T>
        tiny_ptr_base(T const * ptr)
            : base_{reinterpret_cast<std::byte const *>(ptr)}
        {
        }

        auto base_ptr() const -> std::byte const * { return base_; }
    };
    
    //std::unique_ptr<std::byte> tiny_ptr_base::heap_base_{std::make_unique<std::byte>()};
    
    template<class T, class OffsetStorageType = int32_t, size_t OffsetStrideBytes = 1>
    class tiny_ptr
    {
        /*template<class _T, class _O, size_t _S>
        friend auto operator+(tiny_ptr<_T, _O, _S> a, tiny_ptr_base b) -> T *;*/
        //template<class _T, class _O, size_t _S>
        //friend auto operator+(tiny_ptr_base b, tiny_ptr<_T, _O, _S> a) -> T *;
    private:
        OffsetStorageType offset_ = 0;
    public:
        tiny_ptr() = default;

        tiny_ptr(std::nullptr_t)
            : offset_{0}
        {
        }

        tiny_ptr(tiny_ptr_base & b, T const * value)
        {
            b.init_once(value);
            offset_ = static_cast<OffsetStorageType>(std::distance(b.base_, reinterpret_cast<std::byte const *>(value)) / OffsetStrideBytes);
        }

        tiny_ptr(tiny_ptr const &) = default;
        tiny_ptr(tiny_ptr &&) = default;
        
        auto deref(tiny_ptr_base b) -> T *
        {
            return const_cast<T *>(reinterpret_cast<T const *>(std::next(b.base_ptr(), offset_ * OffsetStrideBytes)));
        }
        
        auto deref(tiny_ptr_base b) const -> T const *
        {
            return reinterpret_cast<T const *>(std::next(b.base_ptr(), offset_ * OffsetStrideBytes));
        }

        operator bool() const
        {
            // NULL is always the base's offset
            return offset_ != 0;
        }

        auto operator=(tiny_ptr const &) -> tiny_ptr & = default;
        auto operator=(tiny_ptr &&) -> tiny_ptr & = default;

        auto operator<(tiny_ptr const & p) const -> bool
        {
            return offset_ < p.offset_;
        }
        
        auto operator<=(tiny_ptr const & p) const -> bool
        {
            return offset_ <= p.offset_;
        }
        
        auto operator>(tiny_ptr const & p) const -> bool
        {
            return offset_ > p.offset_;
        }
        
        auto operator>=(tiny_ptr const & p) const -> bool
        {
            return offset_ >= p.offset_;
        }
        
        auto operator!=(tiny_ptr const & p) const -> bool
        {
            return offset_ != p.offset_;
        }
        
        auto operator==(tiny_ptr const & p) const -> bool
        {
            return offset_ == p.offset_;
        }
    };
    
    /*template<class T, class O, size_t S>
    inline auto operator+(tiny_ptr<T, O, S> a, tiny_ptr_base b) -> T *
    {
        return a.deref(b);
    }*/

    template<class T, class O, size_t S>
    inline auto operator+(tiny_ptr_base b, tiny_ptr<T, O, S> a) -> T *
    {
        return a.deref(b);
    }
    
    template<class T>
    inline auto operator-(tiny_ptr_base & b, T * a) -> tiny_ptr<std::remove_cv_t<T>>
    {
        return {b, a};
    }

    template<class T>
    struct global_tiny_ptr_storage final
    {
        static auto base() -> tiny_ptr_base &
        {
            static tiny_ptr_base base{};
            return base;
        }
    };

    template<class T>
    static inline auto to_tiny_ptr(T const * ptr) -> tiny_ptr<std::remove_cv_t<T>>
    {
        return (global_tiny_ptr_storage<std::remove_cv_t<T>>::base() - ptr);
    }

    template<class T>
    static inline auto from_tiny_ptr(tiny_ptr<T> ptr) -> T *
    {
        return (global_tiny_ptr_storage<std::remove_cv_t<T>>::base() + ptr);
    }
}
