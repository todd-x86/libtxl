#pragma once

#include <txl/storage.h>

#include <memory>
#include <vector>

namespace txl
{
    template<class Key, class Value>
    class btree final
    {
    private:
        using kv_pair = std::pair<Key, Value>;

        struct node;

        struct node_data final
        {
            kv_pair data;
            std::unique_ptr<node> right = nullptr;

            node_data(kv_pair && p)
                : data{std::move(p)}
            {
            }
        };
        
        struct node final
        {
            std::unique_ptr<node> left = nullptr;
            std::vector<node_data> values;

            auto left_child(size_t index) -> std::unique_ptr<node> &
            {
                if (index > 0)
                {
                    return values[index-1].right;
                }
                return left;
            }
            
            auto right_child(size_t index) -> std::unique_ptr<node> &
            {
                return values[index].right;
            }

            auto split() -> std::unique_ptr<node>
            {
                /**
                 *  [1, 2, 3]
                 *  / |  |  \
                 * a  b  c   d
                 *
                 *    [2]
                 *   /   \
                 * [1]   [3]
                 * / \   / \
                 *a   b c   d
                 */

                // Take middle and make it its own node
                // Put left and right sides of node as children


                auto middle_it = values.begin() + (values.size() / 2);

                std::unique_ptr<node> new_owner;
                auto & n = new_owner->insert_into(std::move(middle_it->data));
                
                n.right_child(0, 
                
                return new_owner;
            }

            auto find_child(Key const & key) -> node *
            {
                auto it = std::lower_bound(values.begin(), values.end(), key, [](auto const & node, auto const & key) {
                    return node.data.first < key;
                });
                if (it == values.end())
                {
                    // last?
                    return values.back().right.get();
                }
                if (it->data.first < key)
                {
                    return it->right.get();
                }

                // Can't go left any further
                if (it == values.begin())
                {
                    return left.get();
                }

                // Go left by one
                return (it - 1)->right.get();
            }

            auto is_leaf() const -> bool
            {
                return left == nullptr and std::find_if(values.begin(), values.end(), [](auto const & node) { return node.right != nullptr; }) == values.end();
            }

            auto num_values() const -> size_t
            {
                return values.size();
            }

            auto insert_into(kv_pair && data) -> node_data &
            {
                auto it = std::lower_bound(values.begin(), values.end(), data, [](auto const & x, auto const & y) {
                    return x.data.first < y.first;
                });
                return *values.emplace(it, std::move(data));
            }
        };

        auto insert_into_leaf(node & curr, kv_pair && data) -> bool
        {
            if (not curr.is_leaf())
            {
                auto * next = curr.find_child(data.first);
                if (next == nullptr)
                {
                    throw std::runtime_error{"no child found"};
                }
                return insert_into_leaf(*next, std::move(data));
            }
            
            if (curr.num_values() >= degree_)
            {
                // Split and move up
                return false;
            }
            curr.insert_into(std::move(data));

            std::cout << "v TREE-------------------\n";
            for (auto const & v : curr.values)
            {
                std::cout << "{" << v.data.first << ", " << v.data.second << "}\n";
            }
            std::cout << "^ TREE-------------------\n";
            return true;
        }

        std::unique_ptr<node> root_;
        size_t degree_;
    public:
        btree(size_t degree)
            : degree_{degree}
        {
        }

        auto insert(Key && key, Value && value) -> void
        {
            if (root_ == nullptr)
            {
                root_ = std::make_unique<node>();
            }
            if (not insert_into_leaf(*root_, std::make_pair(std::move(key), std::move(value))))
            {
                root_ = root_->split();
            }
        }
    };
}
