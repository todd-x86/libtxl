#pragma once

#include <algorithm>

namespace txl
{
    template<class Value>
    class storage
    {
    private:
        alignas(Value) std::byte val_[sizeof(Value)];
    protected:
        auto ptr() -> Value * { return reinterpret_cast<Value *>(&val_[0]); }
        auto val() -> Value & { return *ptr(); }
        auto ptr() const -> Value const * { return reinterpret_cast<Value const *>(&val_[0]); }
        auto val() const -> Value const & { return *ptr(); }

        auto raw_copy(storage const & s) -> void
        {
            std::copy(&s.val_[0], &s.val_[sizeof(Value)], &val_[0]);
        }
    public:
        storage() = default;

        storage(storage const & s)
        {
            val() = s.val();
        }

        storage(storage && s)
        {
            val() = std::move(s.val());
        }
        
        virtual ~storage() = default;

        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            new (ptr()) Value{std::forward<Args>(args)...};
        }

        auto erase() -> void
        {
            val().~Value();
        }

        auto operator=(storage const & s) -> storage &
        {
            if (&s != this)
            {
                val() = s.val();
            }
            return *this;
        }

        auto operator=(storage && s) -> storage &
        {
            if (&s != this)
            {
                val() = std::move(s.val());
            }
            return *this;
        }

        auto swap(storage & s) -> void
        {
            std::swap(s.val(), val());
        }

        auto operator*() -> Value & { return val(); }
        auto operator*() const -> Value const & { return val(); }
        auto operator->() -> Value * { return ptr(); }
        auto operator->() const -> Value const * { return ptr(); }
    };

    template<class Value>
    class safe_storage : public storage<Value>
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
                this->val() = std::move(s.val());
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
}
