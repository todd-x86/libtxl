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
            // seq_ is a way to deal with the ABA problem
            uint32_t seq_ = 0;
        public:
            head_tag() = default;

            head_tag(tiny_ptr<node> p, uint32_t seq)
                : ptr_{p}
                , seq_{seq}
            {
            }

            auto ptr() const -> tiny_ptr<node>
            {
                return ptr_;
            }

            auto seq() const -> uint32_t
            {
                return seq_;
            }

            auto operator==(head_tag const & ht) const -> bool
            {
                return ht.seq_ == seq_ and ht.ptr_ == ptr_;
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

                return head_tag{n, ht.seq()+1};
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
                head = head_.load(std::memory_order_acquire);
                n->next_ = head.ptr();
                new_head = head_tag{to_tiny_ptr(n), head.seq()+1};
            }
            while (not head_.compare_exchange_weak(head, new_head, std::memory_order_release, std::memory_order_relaxed));

            num_inserts_.fetch_add(1, std::memory_order_relaxed);
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            head_tag head, next;

            do
            {
                head = head_.load(std::memory_order_acquire);

                if (head.ptr())
                {
                    next = head_tag{from_tiny_ptr(head.ptr())->next_, head.seq()+1};
                }
                else
                {
                    next = head_tag{nullptr, head.seq()+1};
                }
            }
            while (not head_.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_relaxed));

            // Safely remove the head
            if (head.ptr())
            {
                auto r_head = from_tiny_ptr(head.ptr());
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
