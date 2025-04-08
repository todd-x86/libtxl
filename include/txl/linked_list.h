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

        struct head_tag final
        {
        private:
            tiny_ptr<node> ptr_ = nullptr;
            bool stable_ = true;
        public:
            head_tag() = default;

            head_tag(tiny_ptr<node> p, bool stable)
                : ptr_{p}
                , stable_{stable}
            {
            }

            auto ptr() const -> tiny_ptr<node>
            {
                return ptr_;
            }

            auto stable() const -> bool
            {
                return stable_;
            }

            auto operator==(head_tag const & ht) const -> bool
            {
                return ht.stable_ == stable_ and ht.ptr_ == ptr_;
            }

            auto operator!=(head_tag const & ht) const -> bool
            {
                return not (*this == ht);
            }
        };

        auto replace_head(tiny_ptr<node> n) -> head_tag
        {
            return atomic_swap(head_, [this, n](auto ht) {
                if (ht.ptr())
                {
                    from_tiny_ptr(ht.ptr())->next_ = n;
                }

                return head_tag{n, true};
            });
        }

        std::atomic<head_tag> head_;
        std::atomic<size_t> num_inserts_;
        std::atomic<size_t> num_pops_;
    public:
        atomic_linked_list()
            : head_{head_tag{}}
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

        auto clear() -> size_t
        {
            size_t num_destroyed = 0;
            auto old_head = atomic_swap(head_, [](auto) { return head_tag{}; }).ptr();
            while (not old_head.is_null())
            {
                auto next = from_tiny_ptr(old_head)->next_;
                delete from_tiny_ptr(old_head);
                ++num_destroyed;
                old_head = next;
            }
            return num_destroyed;
        }

        auto empty() const -> bool
        {
            return head_.load(std::memory_order_relaxed).ptr() == tiny_ptr<node>{nullptr};
        }
        
        auto push_back(Value const & v) -> Value &
        {
            return emplace_back(v);
        }

        template<class... Args>
        auto emplace_back(Args && ... args) -> void
        {
            auto n = new node{ .next_ = tiny_ptr<node>{nullptr}, .canary_ = 123 };
            new (&n->value_[0]) Value{std::forward<Args>(args)...};

            head_tag head{}, new_head{};
            do
            {
                head = head_.load(std::memory_order_relaxed);
                n->next_ = head.ptr();
                new_head = head_tag{to_tiny_ptr(n), true};
            }
            while (not head.stable() or not head_.compare_exchange_weak(head, new_head, std::memory_order_release, std::memory_order_relaxed));

            num_inserts_.fetch_add(1, std::memory_order_relaxed);
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            // Stable/unstable paradigm explanation:
            //    We're faced with the ABA problem when pop_and_release_front().  emplace_back()/push_back() are purely atomic because they 
            //    do not depend on inspecting the next value.  pop_and_release_front() is semi-atomic because it reads the head and pops by
            //    swapping with the head's next node.  If the head is deleted in between the load() and inspecting `next_` for the successive
            //    head, we encounter a `heap-use-after-free` because of the race condition with deletion.
            //
            //    To prevent racing while invoking pop_and_release_front() on several threads, we mark the head as "unstable" which
            //    means it cannot be popped until it is made stable again.  While it's unstable, we can inspect the head safely without
            //    deleting.  During the replacement, we make the head stable again so the stable-to-unstable-to-stable time should be short.
            //
            head_tag old_head, head{}, next{};
            
            // Mark unstable
            do
            {
                head = head_.load(std::memory_order_relaxed);
                old_head = head;
                next = head_tag{head.ptr(), false};
            }
            while (not old_head.stable() or not head_.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_relaxed));

            do
            {
                // head is now unstable

                // Make it stable and swap
                if (old_head.ptr())
                {
                    next = head_tag{from_tiny_ptr(old_head.ptr())->next_, true};
                }
                else
                {
                    next = head_tag{nullptr, true};
                }
            }
            while (not head_.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_relaxed));

            // Safely remove the head
            if (old_head.ptr())
            {
                auto r_head = from_tiny_ptr(old_head.ptr());
                r_head->canary_check();
                auto res = std::make_optional<Value>(std::move(r_head->val()));
                num_pops_.fetch_add(1, std::memory_order_relaxed);
                delete r_head;
                
                return res;
            }
            return {};
        }
    };
}
