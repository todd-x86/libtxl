#pragma once

#include <txl/atomic.h>

#include <atomic>
#include <cassert>
#include <optional>
#include <algorithm>
#include <iterator>
#include <memory>

namespace txl
{
    // NOTE: For ABA problem, can we allocate a chunk of sizeof(Value)*N and use it
    //       like an arena?
    template<class Value>
    class atomic_linked_list final
    {
    private:
        struct value_node final
        {
            Value val;
            value_node * next = nullptr;
            
            template<class... Args>
            value_node(Args && ... args)
                : val(std::forward<Args>(args)...)
            {
            }
        };

        struct node_gen final
        {
            value_node * ptr;
            uint32_t ctr;
        };

        static_assert(sizeof(node_gen) <= sizeof(__uint128_t), "Generational node cannot exceed 16 bytes");
        
        node_gen head_;
        std::atomic<size_t> num_inserts_;
        std::atomic<size_t> num_pops_;

        static inline auto create_alias(value_node * p)
        {
            return node_gen{p, 0};
        }

        static inline auto next_alias(node_gen n, value_node * vn)
        {
            return node_gen{vn, n.ctr+1};
        }

        static inline auto compare_and_swap(node_gen & target, node_gen expected, node_gen desired) -> bool
        {
            return __atomic_compare_exchange(&target, &expected, &desired, true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
        }

        static inline auto load(node_gen const & target) -> node_gen
        {
            node_gen res;
            __atomic_load(&target, &res, __ATOMIC_ACQUIRE);
            return res;
        }

        static inline auto load_relaxed(node_gen const & target) -> node_gen
        {
            node_gen res;
            __atomic_load(&target, &res, __ATOMIC_RELAXED);
            return res;
        }

        static inline auto store(node_gen & target, node_gen val) -> void
        {
            __atomic_store(&target, &val, __ATOMIC_RELEASE);
        }
    public:
        atomic_linked_list()
        {
            store(head_, create_alias(nullptr));
            num_inserts_.store(0, std::memory_order_release);
            num_pops_.store(0, std::memory_order_release);
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
            node_gen empty, head;
            do
            {
                head = load(head_);
                empty = next_alias(head, nullptr);
            }
            while (not compare_and_swap(head_, head, empty));
            
            num_inserts_.store(0, std::memory_order_release);
            num_pops_.store(0, std::memory_order_release);

            if (not head.ptr)
            {
                return 0;
            }

            size_t num_deleted = 0;
            auto p_head = head.ptr;
            while (p_head)
            {
                auto next = p_head->next;
                delete p_head;
                ++num_deleted;
                p_head = next;
            }
            return num_deleted;
        }

        auto empty() const -> bool
        {
            return load_relaxed(head_).ptr == nullptr;
        }
        
        auto push_front(Value const & v) -> void
        {
            emplace_front(v);
        }

        template<class... Args>
        auto emplace_front(Args && ... args) -> void
        {
            auto n = new value_node(std::forward<Args>(args)...);
            node_gen head, next;
            do
            {
                head = load(head_);
                // next points to previous value
                n->next = head.ptr;
                next = next_alias(head, n);
            }
            while (not compare_and_swap(head_, head, next));
            
            num_inserts_.fetch_add(1, std::memory_order_acq_rel);
        }
        
        auto pop_and_release_front() -> std::optional<Value>
        {
            node_gen head, next;
            do
            {
                head = load(head_);
                if (not head.ptr)
                {
                    return {};
                }
                next = next_alias(head, head.ptr->next);
            }
            while (not compare_and_swap(head_, head, next));
            
            if (not head.ptr)
            {
                return {};
            }
            auto res = std::make_optional<Value>(std::move(head.ptr->val));
            delete head.ptr;
            num_pops_.fetch_add(1, std::memory_order_acq_rel);
            return res;
        }
    };
}
