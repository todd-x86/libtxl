#pragma once

#include <memory>
#include <utility>

namespace txl
{
    template<class Key, class Value>
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
        }
    };
}
