#pragma once

#include <atomic>
#include <functional>

namespace txl
{
    /**
     * Performs an atomic swap with acquire-release semantics in a weak memory model.
     * 
     * \tparam T underlying atomic storage type
     * \param value atomic to swap
     * \param new_value replacement value
     * \return previous value in atomic
     */
    template<class T>
    static auto atomic_swap(std::atomic<T> & value, T new_value) -> T
    {
        auto old_value = T{};
        do
        {
            old_value = value.load(std::memory_order_acquire);
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }
    
    /**
     * Performs an atomic swap with a value supplied via a factory function.
     * This operation is performed with acquire-release semantics in a weak memory model.
     * 
     * \tparam T underlying atomic storage type
     * \tparam Func factory function of type: (T const &) -> T
     * \param value atomic to swap
     * \param new_value_factory factory function returning replacement value
     * \return previous value in atomic
     */
    template<class T, class Func>
    static auto atomic_swap(std::atomic<T> & value, Func && new_value_factory) -> T
    {
        auto old_value = T{};
        auto new_value = T{};
        do
        {
            old_value = value.load(std::memory_order_acquire);
            new_value = new_value_factory(old_value);
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }
    
    /**
     * Performs an atomic swap with a new value if a condition is met.
     * This operation is performed with acquire-release semantics in a weak memory model.
     * 
     * \tparam T underlying atomic storage type
     * \tparam Func condition function type: (T const &) -> bool
     * \param value atomic to swap
     * \param new_value replacement value
     * \param cond condition function that returns true to perform a swap
     * \return previous value in atomic
     */
    template<class T, class Func>
    static auto atomic_swap_if(std::atomic<T> & value, T new_value, Func && cond) -> T
    {
        auto old_value = T{};
        do
        {
            old_value = value.load(std::memory_order_relaxed);
            if (not cond(old_value))
            {
                break;
            }
            old_value = value.load(std::memory_order_acquire);
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }
    
    /**
     * Performs an atomic swap with a new value supplied from a factory function if a condition is met.
     * This operation is performed with acquire-release semantics in a weak memory model.
     * 
     * \tparam T underlying atomic storage type
     * \tparam FactoryFunc new value function type: (T const &) -> T
     * \tparam ConditionFunc condition function type: (T const &) -> bool
     * \param value atomic to swap
     * \param new_value_factory replacement value factory function
     * \param cond condition function that returns true to perform a swap
     * \return previous value in atomic
     */
    template<class T, class FactoryFunc, class ConditionFunc>
    static auto atomic_swap_if(std::atomic<T> & value, FactoryFunc && new_value_factory, ConditionFunc && cond) -> T
    {
        auto old_value = T{};
        auto new_value = T{};
        do
        {
            old_value = value.load(std::memory_order_relaxed);
            if (not cond(old_value))
            {
                break;
            }
            old_value = value.load(std::memory_order_acquire);
            new_value = new_value_factory(old_value);
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }

    /**
     * Holds a value from an atomic and performs a compare_exchange_weak()
     * operation if the value has changed since the atomic was acquired.
     *
     * Uses acquire-release semantics.
     *
     * \tparam T atomic value type
     */
    template<class T>
    class acquire_lock final
    {
    private:
        std::atomic<T> & atom_;
        T old_value_;
        T new_value_;
    public:
        /**
         * Constructs a new acquire_lock on an atomic.  The name is a misnomer
         * since no actual locking is performed. A copy of the value is
         * acquired from the supplied atomic and replaced at destruction if
         * the value has changed.
         *
         * \param value atomic value to load and save
         */
        acquire_lock(std::atomic<T> & value)
            : atom_{value}
            , old_value_{value.load(std::memory_order_acquire)}
            , new_value_{old_value_}
        {
        }

        /**
         * Compare-and-exchanges atomic if the value has changed.
         */
        ~acquire_lock()
        {
            if (old_value_ != new_value_)
            {
                atom_.compare_exchange_weak(old_value_, new_value_, std::memory_order_release, std::memory_order_relaxed);
            }
        }
        
        /**
         * Mutable accessor.
         */
        T & value() { return new_value_; }
        /**
         * Const accessor.
         */
        T const & value() const { return new_value_; }

        /**
         * Mutable accessor.
         */
        T & operator*() { return new_value_; }
        /**
         * Const accessor.
         */
        T const & operator*() const { return new_value_; }
        
        /**
         * Mutable accessor.
         */
        T * operator->() { return &new_value_; }
        /**
         * Const accessor.
         */
        T const * operator->() const { return &new_value_; }
    };

    template<class Value>
    class lazy_atomic final
    {
    public:
        using factory_type = std::function<Value*()>;
    private:
        struct init_ready final
        {
        };

        struct node final
        {
            uintptr_t ptr_ = 0x0;

            node() = default;
            // Represents a "pending" state to let the thread that acquired it to initialize
            node(init_ready) : ptr_{0x1} {}
            // Initialization via factory
            node(factory_type & fac) : ptr_{reinterpret_cast<uintptr_t>(fac())} {}
            // Initialization by move
            node(Value && v) : ptr_{reinterpret_cast<uintptr_t>(new Value(std::move(v)))} {}

            auto is_empty() const { return ptr_ == 0x0; }
            auto is_pending() const { return ptr_ == 0x1; }
            auto has_value() const { return not is_empty() and not is_pending(); }

            auto destroy()
            {
                if (is_empty() or is_pending())
                {
                    return;
                }
                auto p = get();
                ptr_ = 0x0;
                delete p;
            }

            auto get() const { return reinterpret_cast<Value const *>(ptr_); }
            auto get() { return reinterpret_cast<Value *>(ptr_); }
        };
        factory_type factory_;
        std::atomic<node> value_;
    public:
        lazy_atomic(factory_type fac)
            : factory_{std::move(fac)}
        {
            // Empty
            value_.store(node{}, std::memory_order_release);
        }

        lazy_atomic(lazy_atomic const &) = delete;
        lazy_atomic(lazy_atomic &&) = delete;

        ~lazy_atomic()
        {
            clear();
        }

        auto clear()
        {
            node expected, desired{};
            // Swap out with an empty node
            do
            {
                expected = value_.load(std::memory_order_acquire);
                if (not expected.has_value())
                {
                    // Nothing to destruct
                    return;
                }
            }
            while (not value_.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_relaxed));
            
            // Destroy contents
            expected.destroy();
        }

        auto operator=(Value && v) -> lazy_atomic &
        {
            clear();

            node expected, desired{init_ready{}};
            // Perform a swap just to reserve the spot -- we'll init later
            bool replaced = false;
            do
            {
                replaced = false;
                expected = value_.load(std::memory_order_acquire);
                if (not expected.is_empty())
                {
                    break;
                }
                replaced = true;
            }
            while (not value_.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_relaxed));

            if (replaced)
            {
                // We captured the spot (already), let's get the pointer or initialize it first
                desired = node{std::move(v)};
                do
                {
                    expected = value_.load(std::memory_order_acquire);
                }
                while (not value_.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_relaxed));
            }
            
            return *this;
        }

        auto ptr() -> Value *
        {
            node expected, desired{init_ready{}};
            // Perform a swap just to reserve the spot -- we'll init later
            bool replaced = false;
            do
            {
                replaced = false;
                expected = value_.load(std::memory_order_acquire);
                if (not expected.is_empty())
                {
                    break;
                }
                replaced = true;
            }
            while (not value_.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_relaxed));

            if (replaced)
            {
                // We captured the spot (already), let's get the pointer or initialize it first
                desired = node{factory_};
                do
                {
                    expected = value_.load(std::memory_order_acquire);
                }
                while (not value_.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_relaxed));
            }
            
            // Spin until we get a node that isn't pending
            do
            {
                expected = value_.load(std::memory_order_relaxed);
            }
            while (expected.is_pending());
            return expected.get();
        }

        auto get() -> Value &
        {
            return *ptr();
        }
    };
}
