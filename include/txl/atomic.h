#pragma once

#include <atomic>

namespace txl
{
    template<class T>
    static auto atomic_swap(std::atomic<T> & value, T new_value) -> T
    {
        auto old_value = T{};
        do
        {
            old_value = value.load(std::memory_order_relaxed);
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }
    
    template<class T, class Func>
    static auto atomic_swap(std::atomic<T> & value, Func && new_value_factory) -> T
    {
        auto old_value = T{};
        auto new_value = T{};
        do
        {
            old_value = value.load(std::memory_order_relaxed);
            new_value = new_value_factory(old_value);
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }
    
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
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }
    
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
            new_value = new_value_factory(old_value);
        }
        while (not value.compare_exchange_weak(old_value, new_value, std::memory_order_release, std::memory_order_relaxed));
        return old_value;
    }
}
