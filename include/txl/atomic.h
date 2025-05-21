#pragma once

#include <atomic>

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

    template<class T>
    class acquire_lock final
    {
    private:
        std::atomic<T> & atom_;
        T old_value_;
        T new_value_;
    public:
        acquire_lock(std::atomic<T> & value)
            : atom_{value}
            , old_value_{value.load(std::memory_order_acquire)}
            , new_value_{old_value_}
        {
        }

        ~acquire_lock()
        {
            if (old_value_ != new_value_)
            {
                atom_.compare_exchange_weak(old_value_, new_value_, std::memory_order_release, std::memory_order_relaxed);
            }
        }
        
        T & value() { return new_value_; }
        T const & value() const { return new_value_; }

        T & operator*() { return new_value_; }
        T const & operator*() const { return new_value_; }
        
        T * operator->() { return &new_value_; }
        T const * operator->() const { return &new_value_; }
    };
}
