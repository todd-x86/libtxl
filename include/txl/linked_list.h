#pragma once

#include <txl/on_exit.h>

#include <atomic>
#include <cassert>
#include <optional>

namespace txl
{
    template<class Value>
    class atomic_linked_list final
    {
    private:
        struct node final
        {
            alignas(Value) std::byte value_[sizeof(Value)];
            node * next_;

            ~node()
            {
                val().~Value();
            }

            auto val() const -> Value const & { return *reinterpret_cast<Value const *>(&value_[0]); }
            auto val() -> Value & { return *reinterpret_cast<Value *>(&value_[0]); }
        };

        struct list_data final
        {
            std::atomic<node *> head_;
            std::atomic<node *> tail_;

            list_data()
                : head_{nullptr}
                , tail_{nullptr}
            {
            }
        };

        std::atomic<list_data *> data_;
        
        auto swap_and_clear(list_data * data) -> void
        {
            data = data_.exchange(data, std::memory_order_acq_rel);
            if (data == nullptr)
            {
                return;
            }

            auto _ = on_exit{[data]() {
                delete data;
            }};
            
            assert(data != nullptr);

            while (true)
            {
                auto head = data->head_.load(std::memory_order_acquire);
                if (head == nullptr)
                {
                    return;
                }

                auto old_head = head;
                auto next = head->next_;
                if (data->head_.compare_exchange_weak(head, next, std::memory_order_acq_rel))
                {
                    delete old_head;
                }
            }
        }
    
        template<class T>
        inline auto unsafe_swap_atomics(std::atomic<T> & x, std::atomic<T> & y) -> void
        {
            // NOTE: This is not a literal atomic swap between two atomics.
            // We can't guarantee two exchanges are atomic, but we can swap
            // and ensure the best case is they do not race.
            while (true)
            {
                auto prev_x = x.load(std::memory_order_acquire);
                auto prev_y = y.load(std::memory_order_acquire);
                if (x.compare_exchange_weak(prev_x, prev_y, std::memory_order_acq_rel))
                {
                    while (not y.compare_exchange_weak(prev_y, prev_x, std::memory_order_acq_rel))
                    {
                        prev_y = y.load(std::memory_order_acquire);
                    }
                    return;
                }
            }
        }
    public:
        atomic_linked_list()
            : data_{new list_data{}}
        {
        }

        // Copying a list is an expensive operation that requires locking the entire list until we're done copying
        atomic_linked_list(atomic_linked_list const &) = delete;

        atomic_linked_list(atomic_linked_list && a)
            : atomic_linked_list()
        {
            unsafe_swap_atomics(data_, a.data_);
        }

        ~atomic_linked_list()
        {
            swap_and_clear(nullptr);
        }
        
        auto operator=(atomic_linked_list const &) -> atomic_linked_list & = delete;

        auto operator=(atomic_linked_list && a) -> atomic_linked_list &
        {
            if (this != &a)
            {
                clear();
                unsafe_swap_atomics(data_, a.data_);
            }
            return *this;
        }

        auto clear() -> void
        {
            swap_and_clear(new list_data{});
        }

        auto empty() const -> bool
        {
            auto data = data_.load(std::memory_order_acquire);
            return data == nullptr or data->tail_.load(std::memory_order_acquire) == nullptr;
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> Value &
        {
            auto n = new node{ .next_ = nullptr };
            new (&n->value_[0]) Value{std::forward<Args>(args)...};
            
            auto data = data_.load(std::memory_order_acquire);
            assert(data != nullptr);

            // Replace or attach to tail
            while (true)
            {
                auto old_tail = data->tail_.load(std::memory_order_acquire);
                if (data->tail_.compare_exchange_weak(old_tail, n, std::memory_order_acq_rel))
                {
                    if (old_tail)
                    {
                        old_tail->next_ = n;
                    }
                    break;
                }
            }

            // Replace head if empty
            node * head = nullptr;
            data->head_.compare_exchange_weak(head, n, std::memory_order_acq_rel);
            return n->val();
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            auto data = data_.load(std::memory_order_acquire);
            assert(data != nullptr);

            while (true)
            {
                auto head = data->head_.load(std::memory_order_acquire);
                if (head == nullptr)
                {
                    return {};
                }

                auto old_head = head;
                auto next = head->next_;
                if (data->head_.compare_exchange_weak(head, next, std::memory_order_acq_rel))
                {
                    // Move tail forward if the head is the same as the tail
                    head = old_head;
                    data->tail_.compare_exchange_strong(head, next, std::memory_order_acq_rel);

                    auto res = std::make_optional<Value>(std::move(old_head->val()));
                    delete old_head;
                    return res;
                }
            }
        }
    };
}
