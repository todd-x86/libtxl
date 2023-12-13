#pragma once

#include <memory>

namespace txl
{
    template<class Value>
    class opaque_ptr
    {
    private:
        static constexpr uint8_t UNIQUE_PTR_MASK = 0x1,
                                 SHARED_PTR_MASK = 0x2,
                                 SMART_PTR_MASK = UNIQUE_PTR_MASK | SHARED_PTR_MASK;
        static constexpr uint64_t PTR_MASK = ~static_cast<uint64_t>(SMART_PTR_MASK);
        union
        {
            std::unique_ptr<Value> uptr_;
            std::shared_ptr<Value> sptr_;
        };
        union
        {
            Value * ptr_ = nullptr;
            uint64_t bits_;
        };

        Value * get_pointer()
        {
            return reinterpret_cast<Value *>(bits_ & PTR_MASK);
        }
        
        Value const * get_pointer() const
        {
            return reinterpret_cast<Value const *>(bits_ & PTR_MASK);
        }

        void apply_mask(uint8_t mask)
        {
            // Confirm mask isn't corrupting memory
            assert((bits_ & mask) == 0);
            bits_ |= mask;
        }

        inline bool is_unique_ptr() const { return (bits_ & UNIQUE_PTR_MASK) != 0; }
        inline bool is_shared_ptr() const { return (bits_ & SHARED_PTR_MASK) != 0; }
    public:
        opaque_ptr() = default;
        opaque_ptr(std::unique_ptr<Value> && p)
            : uptr_(std::move(p))
            , ptr_(uptr_.get())
        {
            apply_mask(UNIQUE_PTR_MASK);
        }
        opaque_ptr(std::shared_ptr<Value> const & p)
            : sptr_(p)
            , ptr_(sptr_.get())
        {
            apply_mask(SHARED_PTR_MASK);
        }
        opaque_ptr(std::shared_ptr<Value> && p)
            : sptr_(std::move(p))
            , ptr_(sptr_.get())
        {
            apply_mask(SHARED_PTR_MASK);
        }
        opaque_ptr(Value * p)
            : ptr_(p)
        {
        }
        ~opaque_ptr()
        {
            if (is_unique_ptr())
            {
                uptr_.~unique_ptr();
            }
            else if (is_shared_ptr())
            {
                sptr_.~shared_ptr();
            }
        }
        opaque_ptr(opaque_ptr const &) = delete;
        opaque_ptr(opaque_ptr &&) = default;

        Value * operator->() { return get_pointer(); }
        Value const * operator->() const { return get_pointer(); }
        Value & operator*() { return *get_pointer(); }
        Value const & operator*() const { return *get_pointer(); }

        operator bool() const { return has_value(); }
        bool has_value() const { return get_pointer() != nullptr; }
    };
}
