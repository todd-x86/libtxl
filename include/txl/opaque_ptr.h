#pragma once

#include <memory>

namespace txl
{
    template<class Value>
    class opaque_ptr
    {
    private:
        enum class storage_type
        {
            none,
            unique,
            shared,
            raw,
        };
        union
        {
            std::unique_ptr<Value> uptr_;
            std::shared_ptr<Value> sptr_;
        };
        Value * ptr_ = nullptr;
        storage_type storage_ = storage_type::none;
    public:
        opaque_ptr() = default;
        opaque_ptr(std::unique_ptr<Value> && p)
            : uptr_(std::move(p))
            , ptr_(uptr_.get())
            , storage_(storage_type::unique)
        {
        }
        opaque_ptr(std::shared_ptr<Value> const & p)
            : sptr_(p)
            , ptr_(sptr_.get())
            , storage_(storage_type::shared)
        {
        }
        opaque_ptr(std::shared_ptr<Value> && p)
            : sptr_(std::move(p))
            , ptr_(sptr_.get())
            , storage_(storage_type::shared)
        {
        }
        opaque_ptr(Value * p)
            : ptr_(p)
            , storage_(storage_type::raw)
        {
        }
        ~opaque_ptr()
        {
            switch (storage_)
            {
                case storage_type::unique:
                    uptr_.~unique_ptr();
                    break;
                case storage_type::shared:
                    sptr_.~shared_ptr();
                    break;
                case storage_type::raw:
                case storage_type::none:
                    break;
            }
        }
        opaque_ptr(opaque_ptr const &) = delete;
        opaque_ptr(opaque_ptr &&) = default;

        Value * operator->() { return ptr_; }
        Value const * operator->() const { return ptr_; }
        Value & operator*() { return *ptr_; }
        Value const & operator*() const { return *ptr_; }

        operator bool() const { return has_value(); }
        bool has_value() const { return ptr_ != nullptr; }
    };
}
