#pragma once

#include <txl/atomic.h>

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

        auto replace_head(node * n) -> node *
        {
            return atomic_swap_if(head_, n, [](auto node) { return node == nullptr; });
        }

        auto replace_tail(node * n) -> node *
        {
            return atomic_swap(tail_, [n](auto tail) {
                if (tail)
                {
                    tail->next_ = n;
                }
                return n;
            });
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
        atomic_linked_list(atomic_linked_list &&) = delete;

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
            replace_tail(nullptr);
            auto old_head = replace_head(nullptr);
            while (old_head)
            {
                auto next = old_head->next_;
                delete old_head;
                old_head = next;
            }
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
            //printf("NEW: %p\n", n);
            new (&n->value_[0]) Value{std::forward<Args>(args)...};
            
            replace_tail(n);
            replace_head(n);
            
            num_inserts_.fetch_add(1, std::memory_order_relaxed);

            n->canary_check();
            return n->val();
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            auto old_head = atomic_swap_if(head_, [](auto h) { return h->next_; }, [](auto h) { return h != nullptr; });
            if (old_head)
            {
                atomic_swap_if(tail_, [](auto t) { return t->next_; }, [old_head](auto t) { return old_head == t; });
                old_head->canary_check();
                auto res = std::make_optional<Value>(std::move(old_head->val()));
                num_pops_.fetch_add(1, std::memory_order_relaxed);
                //printf("DEL: %p\n", old_head);
                delete old_head;
                return res;
            }
            return {};
        }
    };
}
