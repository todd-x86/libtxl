#pragma once

#include <txl/atomic.h>
#include <txl/tiny_ptr.h>

#include <atomic>
#include <cassert>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>

namespace txl
{
    template<class Value>
    class atomic_linked_list final
    {
    private:
        struct node final
        {
            alignas(Value) std::byte value_[sizeof(Value)];
            tiny_ptr<node> next_;
            uint8_t canary_ = 123;

            node(node const &) = delete;
            node(node &&) = delete;

            ~node()
            {
                canary_ = 0;
                val().~Value();
            }
            
            template<class... Args>
            auto assign(Args && ... args) -> void
            {
                new (&value_[0]) Value{std::forward<Args>(args)...};
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

        auto replace_head(tiny_ptr<node> n) -> tiny_ptr<node>
        {
            return atomic_swap(head_, [this, n](auto tail) {
                if (tail)
                {
                    from_tiny_ptr(tail)->next_ = n;
                }
                return n;
            });
        }

        std::atomic<tiny_ptr<node>> head_;
        std::atomic<size_t> num_inserts_;
        std::atomic<size_t> num_pops_;
    public:
        atomic_linked_list()
            : head_{nullptr}
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
            auto old_head = replace_head(tiny_ptr<node>{nullptr});
            while (old_head)
            {
                auto next = from_tiny_ptr(old_head)->next_;
                delete from_tiny_ptr(old_head);
                old_head = next;
            }
        }

        auto empty() const -> bool
        {
            return head_.load(std::memory_order_relaxed) == tiny_ptr<node>{nullptr};
        }
        
        auto push_back(Value const & v) -> Value &
        {
            return emplace_back(v);
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> Value &
        {
            auto n = new node{ .next_ = tiny_ptr<node>{nullptr}, .canary_ = 123 };
            //printf("NEW: %p\n", n);
            new (&n->value_[0]) Value{std::forward<Args>(args)...};

            auto tn = to_tiny_ptr(n);

            tiny_ptr<node> head = nullptr;
            do
            {
                head = head_.load(std::memory_order_relaxed);
                n->next_ = head;
            }
            while (not head_.compare_exchange_weak(head, tn, std::memory_order_release, std::memory_order_relaxed));

            num_inserts_.fetch_add(1, std::memory_order_relaxed);

            n->canary_check();
            return n->val();
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            tiny_ptr<node> head = nullptr, next = nullptr;
            do
            {
                head = head_.load(std::memory_order_relaxed);
                if (head)
                {
                    next = from_tiny_ptr(head)->next_;
                }
            }
            while (not head_.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_relaxed));

            if (head)
            {
                //atomic_swap_if(tail_, [](auto t) { return t->next_; }, [old_head](auto t) { return old_head == t; });
                auto r_head = from_tiny_ptr(head);
                r_head->canary_check();
                auto res = std::make_optional<Value>(std::move(r_head->val()));
                num_pops_.fetch_add(1, std::memory_order_relaxed);
                //printf("DEL: %p\n", old_head);
                delete r_head;
                return res;
            }
            return {};
        }
    };
}
