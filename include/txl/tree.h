#pragma once

#include <memory>
#include <utility>

namespace txl
{
    template<class Key, class Value, template<class> class Less = std::less>
    class binary_search_tree final
    {
    private:
        struct node;
        using node_ptr = std::unique_ptr<node>;
        using kv_pair = std::pair<Key, Value>;

        struct node final
        {
            kv_pair data_;
            std::pair<node_ptr, node_ptr> children_;

            node(kv_pair && data)
                : data_{std::move(data)}
                , children_{nullptr, nullptr}
            {
            }

            auto left() const
            {
                return children_.first.get();
            }

            auto right() const
            {
                return children_.second.get();
            }

            auto is_leaf() const
            {
                return left() == nullptr and right() == nullptr;
            }
        };

        node_ptr root_;
    public:
        auto emplace(Key && key, Value && value) -> void
        {
            auto data = std::make_pair(std::move(key), std::move(value));
            if (root_ == nullptr)
            {
                root_ = std::make_unique<node>(std::move(data));
                return;
            }

            auto cmp = Less<Key>{};

            auto * node = root_.get();
            while (node and not node->is_leaf())
            {
                auto key_lt = cmp(key, node->data_.first);
                if (key_lt)
                {
                    node = node->left();
                }
                else
                {
                    node = node->right();
                }
            }
            if (not node)
            {
                return;
            }
            if (cmp(key, node->data_.first))
            {
                node->children_.first = std::make_unique<node>(std::move(data));
            }
            else
            {
                node->children_.second = std::make_unique<node>(std::move(data));
            }
        }
    };
}
