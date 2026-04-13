#pragma once

#include <memory>

namespace txl
{
    template<class Value>
    class box
    {
    private:
        static constexpr const uintptr_t PTR_MASK_ADD = 1,
                                         PTR_MASK_RM = ~1;
        Value * ptr_ = nullptr;

        auto get_pointer() const -> Value *
        {
            auto res = reinterpret_cast<uintptr_t>(ptr_);
            return reinterpret_cast<Value *>(res & PTR_MASK_RM);
        }
        
        auto set_mask() -> void
        {
            // Confirm mask isn't corrupting memory
            ptr_ = reinterpret_cast<Value *>(reinterpret_cast<uintptr_t>(ptr_) | PTR_MASK_ADD);
        }

        inline auto is_owned() const -> bool { return (reinterpret_cast<uintptr_t>(ptr_) & PTR_MASK_ADD) != 0; }
    public:
        box() = default;
        box(std::unique_ptr<Value> && p)
            : box(p.release(), true)
        {
        }
        box(std::unique_ptr<Value> const & p)
            : box(p.get(), false)
        {
        }
        box(std::shared_ptr<Value> const & p)
            : box(p.get(), false)
        {
        }
        box(Value * p, bool owned = false)
            : ptr_(p)
        {
            if (owned)
            {
                set_mask();
            }
        }
        box(box && b)
            : ptr_{b.ptr_}
        {
            b.ptr_ = nullptr;
        }

        ~box()
        {
            erase();
        }
        box(box const &) = delete;
        
        auto operator=(box && b) -> box &
        {
            if (this != &b)
            {
                erase();
                std::swap(ptr_, b.ptr_);
            }
            return *this;
        }

        auto erase()
        {
            if (is_owned())
            {
                delete get_pointer();
                ptr_ = nullptr;
            }
        }

        Value * operator->() { return get_pointer(); }
        Value const * operator->() const { return get_pointer(); }
        Value & operator*() { return *get_pointer(); }
        Value const & operator*() const { return *get_pointer(); }

        operator bool() const { return has_value(); }
        bool has_value() const { return get_pointer() != nullptr; }
        auto get() const -> Value const * { return get_pointer(); }
        auto get() -> Value * { return get_pointer(); }
    };
}
