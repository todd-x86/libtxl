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
            uint8_t canary_ = 123;

            node(node const &) = delete;
            node(node &&) = delete;

            ~node()
            {
                canary_ = 0;
                val().~Value();
            }

            auto operator=(node const &) -> node & = delete;
            auto operator=(node &&) -> node & = delete;

            auto canary_check() const
            {
                if (canary_ != 123)
                {
                    std::ostringstream ss;
                    ss << "Heap after use: " << static_cast<void const *>(this) << "\n";
                    throw std::runtime_error{ss.str()};
                }
            }

            auto val() const -> Value const & { return *reinterpret_cast<Value const *>(&value_[0]); }
            auto val() -> Value & { return *reinterpret_cast<Value *>(&value_[0]); }
        };

        auto replace_head(node * n) -> void
        {
            node * head = nullptr;
            do
            {
                head = head_.load(std::memory_order_relaxed);
                if (head != nullptr)
                {
                    break;
                }
            }
            while (not head_.compare_exchange_weak(head, n, std::memory_order_release, std::memory_order_relaxed));
        }

        auto replace_tail(node * n) -> void
        {
            node * tail = nullptr;
            do
            {
                tail = tail_.load(std::memory_order_relaxed);
            }
            while (not tail_.compare_exchange_weak(tail, n, std::memory_order_release, std::memory_order_relaxed));

            if (tail)
            {
                tail->next_ = n;
            }
        }

        std::atomic<node *> head_;
        std::atomic<node *> tail_;
        std::atomic<size_t> num_inserts_;
        std::atomic<size_t> num_pops_;
    public:
        atomic_linked_list()
            : head_{nullptr}
            , tail_{nullptr}
            , num_inserts_{0}
            , num_pops_{0}
        {
        }

        // Copying a list is an expensive operation that requires locking the entire list until we're done copying
        atomic_linked_list(atomic_linked_list const &) = delete;

        atomic_linked_list(atomic_linked_list && a)
            : tail_()
        {
        }

        ~atomic_linked_list()
        {
            clear();
        }
        
        auto operator=(atomic_linked_list const &) -> atomic_linked_list & = delete;
        auto operator=(atomic_linked_list && a) -> atomic_linked_list & = delete;

        auto num_inserts() const -> size_t
        {
            return num_inserts_.load(std::memory_order_relaxed);
        }
        
        auto num_pops() const -> size_t
        {
            return num_pops_.load(std::memory_order_relaxed);
        }

        auto clear() -> void
        {
        }

        auto empty() const -> bool
        {
            return head_.load(std::memory_order_relaxed) == nullptr;
        }
        
        auto push_back(Value const & v) -> Value &
        {
            return emplace_back(v);
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> Value &
        {
            auto n = new node{ .next_ = nullptr, .canary_ = 123 };
            printf("NEW: %p\n", n);
            new (&n->value_[0]) Value{std::forward<Args>(args)...};
            
            replace_head(n);
            replace_tail(n);
            
            num_inserts_.fetch_add(1, std::memory_order_relaxed);

            n->canary_check();
            return n->val();
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            auto head = head_.load(std::memory_order_relaxed);
            while (head)
            {
                if (head_.compare_exchange_weak(head, head->next_, std::memory_order_release, std::memory_order_relaxed))
                {
                    head->canary_check();
                    auto res = std::make_optional<Value>(std::move(head->val()));
                    num_pops_.fetch_add(1, std::memory_order_relaxed);
                    printf("DEL: %p\n", head);
                    delete head;
                    return res;
                }
                head = head_.load(std::memory_order_relaxed);
            }
            return {};
        }
    };
}
