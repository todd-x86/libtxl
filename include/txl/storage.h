#pragma once

namespace txl
{
    template<class Value>
    class storage
    {
    private:
        alignas(Value) std::byte val_[sizeof(Value)];
        bool emplaced_ = false;
        auto ptr() -> Value * { return reinterpret_cast<Value *>(&val_[0]); }
        auto val() -> Value & { return *ptr(); }
        auto ptr() const -> Value const * { return reinterpret_cast<Value const *>(&val_[0]); }
        auto val() const -> Value const & { return *ptr(); }

        auto raw_copy(storage const & s) -> void
        {
            std::copy(&s.val_[0], &s.val_[sizeof(Value)], &val_[0]);
        }

        auto copy(storage const & s) -> void
        {
            erase();
            if (s.emplaced_)
            {
                val() = s.val();
                emplaced_ = true;
            }
            else
            {
                raw_copy(s);
            }
        }

        auto move(storage && s) -> void
        {
            erase();
            if (s.emplaced_)
            {
                val() = std::move(s.val());
                std::swap(emplaced_, s.emplaced_);
            }
            else
            {
                raw_copy(s);
            }
        }
    public:
        storage() = default;

        storage(storage const & s)
        {
            copy(s);
        }

        storage(storage && s)
        {
            move(std::move(s));
        }
        
        virtual ~storage()
        {
            erase();
        }

        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            erase();
            new (ptr()) Value{std::forward<Args>(args)...};
            emplaced_ = true;
        }

        auto erase() -> bool
        {
            auto can_erase = emplaced_;
            emplaced_ = false;
            if (can_erase)
            {
                val().~Value();
            }
            return can_erase;
        }

        auto empty() const -> bool { return not emplaced_; }

        auto operator=(storage const & s) -> storage &
        {
            if (&s != this)
            {
                copy(s);
            }
            return *this;
        }

        auto operator=(storage && s) -> storage &
        {
            if (&s != this)
            {
                move(std::move(s));
            }
            return *this;
        }

        auto operator*() -> Value & { return val(); }
        auto operator*() const -> Value const & { return val(); }
        auto operator->() -> Value * { return ptr(); }
        auto operator->() const -> Value const * { return ptr(); }
    };
}
