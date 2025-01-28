#pragma once

#include <algorithm>
#include <stdexcept>

// unique_ptr with virtual destructor for deferring destruction (owned vs non-owned pointers)

namespace txl
{
    namespace detail
    {
        template<class T>
        struct virtual_holder
        {
            T * pointer_;

            virtual_holder(T * ptr)
                : pointer_(ptr)
            {
            }

            virtual ~virtual_holder() = default;

            virtual auto copy() -> virtual_holder<T> *
            {
                return new virtual_holder<T>{pointer_};
            }

            virtual auto free() -> void
            {
                delete this;
            }
        };

        template<class T>
        struct heap_holder : virtual_holder<T>
        {
            using virtual_holder<T>::virtual_holder;
            
            auto copy() -> heap_holder<T> * override
            {
                throw std::runtime_error{"cannot copy a heap-backed virtual_ptr"};
            }

            auto free() -> void override
            {
                auto p = this->pointer_;
                this->pointer_ = nullptr;
                delete p;
                
                delete this;
            }
        };
    }

    template<class T>
    class virtual_ptr
    {
    protected:
        detail::virtual_holder<T> * data_ = nullptr;
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
            if (data_)
            {
                data_->free();
            }
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
                data->free();
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
                    data_->free();
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
        heap_ptr(T * value)
            : virtual_ptr<T>()
        {
            this->data_ = new detail::heap_holder<T>{value};
        }
        heap_ptr(heap_ptr const &) = delete;
        heap_ptr(heap_ptr &&) = default;

        auto operator=(heap_ptr const &) -> heap_ptr & = delete;
        auto operator=(heap_ptr && p) -> heap_ptr & = default;
    };

    template<class T, class... Args>
    inline auto make_heap_ptr(Args && ... args) -> heap_ptr<T>
    {
        return heap_ptr<T>{new T{std::forward<Args>(args)...}};
    }
}
