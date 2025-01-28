#pragma once

namespace txl
{
    template<class T>
    class virtual_ptr
    {
    protected:
        T * pointer_ = nullptr;
    public:
        virtual_ptr() = default;
        
        virtual_ptr(T * value)
            : pointer_(value)
        {
        }

        virtual ~virtual_ptr() = default;

        auto operator*() -> T &
        {
            return *pointer_;
        }

        auto operator*() const -> T const &
        {
            return *pointer_;
        }

        auto operator->() -> T *
        {
            return pointer_;
        }

        auto operator->() const -> T const *
        {
            return pointer_;
        }
    };

    template<class T>
    struct heap_ptr : virtual_ptr<T>
    {
        heap_ptr() = default;
        heap_ptr(T * value)
            : virtual_ptr<T>(value)
        {
        }
        heap_ptr(heap_ptr const &) = delete;
        heap_ptr(heap_ptr && p)
            : virtual_ptr<T>()
        {
            std::swap(p.pointer_, this->pointer_);
        }

        ~heap_ptr()
        {
            auto p = this->pointer_;
            this->pointer_ = nullptr;
            delete p;
        }

        auto operator=(heap_ptr const &) -> heap_ptr & = delete;
        auto operator=(heap_ptr && p) -> heap_ptr &
        {
            if (&p != this)
            {
                std::swap(p.pointer_, this->pointer_);
            }
            return *this;
        }
    };

    template<class T, class... Args>
    inline auto make_heap_ptr(Args && ... args) -> heap_ptr<T>
    {
        return heap_ptr<T>{new T{std::forward<Args>(args)...}};
    }
}
