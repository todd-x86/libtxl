#pragma once

#include <algorithm>
#include <stdexcept>

// unique_ptr with virtual destructor for deferring destruction (owned vs non-owned pointers)

namespace txl
{
    namespace detail
    {
        template<class T>
        struct holder_base
        {
            virtual ~holder_base() = default;

            virtual auto ptr() -> T * = 0;
            virtual auto copy() -> holder_base<T> * = 0;
        };

        template<class T>
        struct virtual_holder final : holder_base<T>
        {
            T * pointer_;

            virtual_holder(T * ptr)
                : pointer_(ptr)
            {
            }

            virtual ~virtual_holder() = default;

            auto ptr() -> T * override
            {
                return pointer_;
            }

            auto copy() -> virtual_holder<T> * override
            {
                return new virtual_holder<T>{pointer_};
            }
        };

        template<class T>
        struct heap_holder final : holder_base<T>
        {
            T data_;

            template<class... Args>
            heap_holder(Args && ... args)
                : data_(std::forward<Args>(args)...)
            {
            }
            
            auto ptr() -> T * override
            {
                return &data_;
            }
            
            auto copy() -> heap_holder<T> * override
            {
                throw std::runtime_error{"cannot copy a heap-backed virtual_ptr"};
            }
        };
    }

    template<class T>
    class virtual_ptr
    {
    protected:
        detail::holder_base<T> * data_ = nullptr;
    public:
        virtual_ptr() = default;
        
        virtual_ptr(T * value)
        {
            data_ = new detail::virtual_holder<T>{value};
        }

        virtual_ptr(virtual_ptr const & p)
            : virtual_ptr()
        {
            if (p.data_)
            {
                auto new_data = p.data_->copy();
                reset();
                data_ = new_data;
            }
            else
            {
                reset();
            }
        }
        virtual_ptr(virtual_ptr && p)
            : virtual_ptr()
        {
            std::swap(data_, p.data_);
        }

        ~virtual_ptr()
        {
            reset();
        }

        auto reset() -> void
        {
            auto data = data_;
            data_ = nullptr;
            if (data)
            {
                delete data;
            }
        }
        
        auto operator=(virtual_ptr const & p) -> virtual_ptr &
        {
            if (&p != this)
            {
                if (p.data_)
                {
                    auto new_data = p.data_->copy();
                    reset();
                    data_ = new_data;
                }
                else
                {
                    reset();
                }
            }
            return *this;
        }

        auto operator=(virtual_ptr && p) -> virtual_ptr &
        {
            if (&p != this)
            {
                if (data_)
                {
                    delete data_;
                    data_ = nullptr;
                }
                std::swap(data_, p.data_);
            }
            return *this;
        }

        auto operator*() -> T &
        {
            return *(data_->pointer_);
        }

        auto operator*() const -> T const &
        {
            return *(data_->pointer_);
        }

        auto operator->() -> T *
        {
            return data_ ? data_->pointer_ : nullptr;
        }

        auto operator->() const -> T const *
        {
            return data_ ? data_->pointer_ : nullptr;
        }
    };

    // heap_ptr is a virtual_ptr that owns the memory and destruction
    template<class T>
    struct heap_ptr : virtual_ptr<T>
    {
        heap_ptr() = default;

        template<class... Args>
        heap_ptr(Args && ... args)
            : virtual_ptr<T>()
        {
            this->data_ = new detail::heap_holder<T>{std::forward<Args>(args)...};
        }
        heap_ptr(heap_ptr const &) = delete;
        heap_ptr(heap_ptr &&) = default;

        auto operator=(heap_ptr const &) -> heap_ptr & = delete;
        auto operator=(heap_ptr && p) -> heap_ptr & = default;
    };

    template<class T, class... Args>
    inline auto make_heap_ptr(Args && ... args) -> heap_ptr<T>
    {
        return heap_ptr<T>{std::forward<Args>(args)...};
    }
}
