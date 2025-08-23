#pragma once

#include <algorithm>

namespace txl
{
    template<class>
    class storage_base;

    template<class Value>
    struct storage_value_move final
    {
        static auto move(storage_base<Value> & src, storage_base<Value> & dst) -> void;
    };

    template<class Value>
    struct storage_raw_move final
    {
        static auto move(storage_base<Value> & src, storage_base<Value> & dst) -> void;
    };

    template<class Value>
    class storage_base
    {
        template<class>
        friend struct storage_raw_move;
        template<class>
        friend struct storage_value_move;
    private:
        alignas(Value) std::byte val_[sizeof(Value)];
    protected:
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

    template<class Value, class MovePolicy = storage_value_move<Value>>
    struct storage : storage_base<Value>
    {
        storage() = default;

        storage(storage const & s)
        {
            this->val() = s.val();
        }

        storage(storage && s)
        {
            MovePolicy::move(s, *this);
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

        auto operator=(storage const & s) -> storage &
        {
            if (&s != this)
            {
                this->val() = s.val();
            }
            return *this;
        }

        auto operator=(storage && s) -> storage &
        {
            if (&s != this)
            {
                MovePolicy::move(s, *this);
            }
            return *this;
        }

        auto swap(storage & s) -> void
        {
            std::swap(s.val(), this->val());
        }
    };
    
    template<class Value>
    auto storage_value_move<Value>::move(storage_base<Value> & src, storage_base<Value> & dst) -> void
    {
        new (dst.ptr()) Value{std::move(src.val())};
    }

    template<class Value>
    auto storage_raw_move<Value>::move(storage_base<Value> & src, storage_base<Value> & dst) -> void
    {
        dst.raw_swap(src);
    }

    template<class Value, class MovePolicy = storage_value_move<Value>>
    class safe_storage : protected storage<Value, MovePolicy>
    {
    private:
        bool emplaced_ = false;

        auto copy(safe_storage const & s) -> void
        {
            erase();
            if (s.emplaced_)
            {
                this->val() = s.val();
                emplaced_ = true;
            }
            else
            {
                this->raw_copy(s);
            }
        }

        auto move(safe_storage && s) -> void
        {
            erase();
            if (s.emplaced_)
            {
                MovePolicy::move(s, *this);
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
        
        virtual ~safe_storage()
        {
            erase();
        }

        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            erase();
            new (this->ptr()) Value{std::forward<Args>(args)...};
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

        using storage<Value, MovePolicy>::operator*;
        using storage<Value, MovePolicy>::operator->;

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

    template<class V, class M>
    inline auto operator==(safe_storage<V, M> const & x, safe_storage<V, M> const & y) -> bool
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
