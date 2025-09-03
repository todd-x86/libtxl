#pragma once

// `unsafe_storage` is essentially a `std::optional`-like replacement that shaves off the extra bytes incurred from storing a flag to represent whether a value is contained within it

#include <txl/buffer_ref.h>

#include <array>
#include <algorithm>

namespace txl
{
    template<class Value>
    class storage_base
    {
    private:
        alignas(Value) std::array<std::byte, sizeof(Value)> val_;
    protected:
        auto to_buffer_ref() -> buffer_ref { return {val_}; }
        auto to_buffer_ref() const -> buffer_ref { return {val_}; }
        auto is_zero() const -> bool { return to_buffer_ref().is_zero(); }
        auto fill_zero() -> void { to_buffer_ref().fill(std::byte{0}); }

        auto ptr() -> Value * { return reinterpret_cast<Value *>(&val_[0]); }
        auto val() -> Value & { return *ptr(); }
        auto ptr() const -> Value const * { return reinterpret_cast<Value const *>(&val_[0]); }
        auto val() const -> Value const & { return *ptr(); }

        auto raw_copy(storage_base const & s) -> void
        {
            std::copy(&s.val_[0], &s.val_[sizeof(Value)], &val_[0]);
        }

        auto raw_swap(storage_base & s) -> void
        {
            std::swap(s.val_, val_);
        }
    public:
        auto operator*() -> Value & { return val(); }
        auto operator*() const -> Value const & { return val(); }
        auto operator->() -> Value * { return ptr(); }
        auto operator->() const -> Value const * { return ptr(); }
    };

    template<class Value>
    struct unsafe_storage : storage_base<Value>
    {
        unsafe_storage() = default;

        unsafe_storage(unsafe_storage const & s)
        {
            new (this->ptr()) Value( s.val() );
        }

        unsafe_storage(unsafe_storage && s)
        {
            new (this->ptr()) Value(std::move(s.val()));
        }
        
        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            new (this->ptr()) Value(std::forward<Args>(args)...);
        }

        auto erase() -> void
        {
            this->val().~Value();
        }

        auto operator=(unsafe_storage const & s) -> unsafe_storage &
        {
            if (&s != this)
            {
                new (this->ptr()) Value(s.val());
            }
            return *this;
        }

        auto operator=(unsafe_storage && s) -> unsafe_storage &
        {
            if (&s != this)
            {
                new (this->ptr()) Value(std::move(s.val()));
            }
            return *this;
        }
    };
    

    // this is basically std::optional again
    template<class Value>
    class safe_storage : protected unsafe_storage<Value>
    {
    private:
        bool emplaced_ = false;

        auto copy(safe_storage const & s) -> void
        {
            erase();
            if (s.emplaced_)
            {
                new (this->ptr()) Value(s.val());
                emplaced_ = true;
            }
        }

        auto move(safe_storage && s) -> void
        {
            erase();
            if (s.emplaced_)
            {
                new (this->ptr()) Value(std::move(s.val()));
                std::swap(emplaced_, s.emplaced_);
            }
            else
            {
                this->raw_copy(s);
            }
        }
    public:
        safe_storage() = default;

        safe_storage(safe_storage const & s)
        {
            copy(s);
        }

        safe_storage(safe_storage && s)
        {
            move(std::move(s));
        }
        
        ~safe_storage()
        {
            erase();
        }

        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            erase();
            new (this->ptr()) Value(std::forward<Args>(args)...);
            emplaced_ = true;
        }

        auto erase() -> bool
        {
            auto can_erase = emplaced_;
            emplaced_ = false;
            if (can_erase)
            {
                this->val().~Value();
            }
            return can_erase;
        }

        auto empty() const -> bool { return not emplaced_; }

        using unsafe_storage<Value>::operator*;
        using unsafe_storage<Value>::operator->;

        auto operator=(safe_storage const & s) -> safe_storage &
        {
            if (&s != this)
            {
                copy(s);
            }
            return *this;
        }

        auto operator=(safe_storage && s) -> safe_storage &
        {
            if (&s != this)
            {
                move(std::move(s));
            }
            return *this;
        }
    };

    template<class V>
    inline auto operator==(safe_storage<V> const & x, safe_storage<V> const & y) -> bool
    {
        if (x.empty() != y.empty())
        {
            return false;
        }
        if (x.empty())
        {
            return true;
        }
        return *x == *y;
    }
}
