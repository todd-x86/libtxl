#pragma once

#include <atomic>
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
            //char pad_[64];

            ~node()
            {
                val().~Value();
            }

            auto val() const -> Value const & { return *reinterpret_cast<Value const *>(&value_[0]); }
            auto val() -> Value & { return *reinterpret_cast<Value *>(&value_[0]); }
        };
        // TODO: make this an atomic unique_ptr
        std::atomic<node *> head_;
        std::atomic<node *> tail_;
    public:
        atomic_linked_list()
            : head_(nullptr)
            , tail_(nullptr)
        {
        }

        // Copying a list is an expensive operation that requires locking the entire list until we're done copying
        atomic_linked_list(atomic_linked_list const &) = delete;

        atomic_linked_list(atomic_linked_list && a)
            : atomic_linked_list()
        {
            // This can race, fix it...
            head_.exchange(a.head_, std::memory_order_acq_rel);
            tail_.exchange(a.tail_, std::memory_order_acq_rel);
        }

        ~atomic_linked_list()
        {
            clear();
        }
        
        auto operator=(atomic_linked_list const &) -> atomic_linked_list & = delete;

        auto operator=(atomic_linked_list && a) -> atomic_linked_list &
        {
            if (this != &a)
            {
                // This all can race, fix it...
                // TODO: clear here but swap out first
                clear();
                head_.exchange(a.head_, std::memory_order_acq_rel);
                tail_.exchange(a.tail_, std::memory_order_acq_rel);
            }
            return *this;
        }

        auto clear() -> void
        {
            while (not empty())
            {
                pop_front();
            }
        }

        auto empty() const -> bool
        {
            return tail_.load(std::memory_order_acquire) == nullptr;
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> Value &
        {
            auto n = new node{ .next_ = nullptr };
            new (&n->value_[0]) Value{std::forward<Args>(args)...};

            // Replace or attach to tail
            while (true)
            {
                auto old_tail = tail_.load(std::memory_order_acquire);
                if (tail_.compare_exchange_weak(old_tail, n, std::memory_order_acq_rel))
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
            head_.compare_exchange_weak(head, n, std::memory_order_acq_rel);
            return n->val();
        }

        auto front() const -> Value const &
        {
            auto top = head_.load(std::memory_order_acquire);
            return top->val();
        }

        auto front() -> Value &
        {
            auto top = head_.load(std::memory_order_acquire);
            return top->val();
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            while (true)
            {
                auto head = head_.load(std::memory_order_acquire);
                if (head == nullptr)
                {
                    return {};
                }

                auto old_head = head;
                auto next = head->next_;
                if (head_.compare_exchange_weak(head, next, std::memory_order_acq_rel))
                {
                    // Move tail forward if the head is the same as the tail
                    head = old_head;
                    tail_.compare_exchange_strong(head, next, std::memory_order_acq_rel);

                    auto res = std::make_optional<Value>(std::move(old_head->val()));
                    delete old_head;
                    return res;
                }
            }
        }

        auto pop_front() -> void
        {
            while (true)
            {
                auto head = head_.load(std::memory_order_acquire);
                if (head == nullptr)
                {
                    return;
                }

                auto old_head = head;
                auto next = head->next_;
                if (head_.compare_exchange_weak(head, next, std::memory_order_acq_rel))
                {
                    // Move tail forward if the head is the same as the tail
                    head = old_head;
                    tail_.compare_exchange_strong(head, next, std::memory_order_acq_rel);
            
                    delete old_head;
                    return;
                }
            }
        }
    };
}
