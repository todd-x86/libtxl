#pragma once

#include <txl/vector.h>

#include <memory>
#include <optional>
#include <stdexcept>

namespace txl
{
    template<class Key, class Value>
    class btree final
    {
    private:
        using kv_pair = std::pair<Key, Value>;

        struct node;

        using node_ptr = std::unique_ptr<node>;

        struct index_of_result final
        {
            size_t index;
            bool found;
        };

        struct remove_result final
        {
            bool found;
            bool rebalance;
        };

        struct node final
        {
            txl::vector<node_ptr> children;
            txl::vector<kv_pair> values;
        
            auto find(Key const & key) const -> Value const *
            {
                auto res = index_of(key);
                if (res.found)
                {
                    return &values.at(res.index).second;
                }
                if (is_leaf())
                {
                    return nullptr;
                }
                return children.at(res.index)->find(key);
            }

            auto remove_at(size_t index) -> void
            {
                if (index >= values.size())
                {
                    throw std::runtime_error{"wrong index"};
                }
                values.erase_at(index);
            }

            auto remove_child_at(size_t index) -> void
            {
                if (index >= children.size())
                {
                    throw std::runtime_error{"wrong index"};
                }
                children.erase(children.begin() + index);
            }

            auto set_child(size_t index, node_ptr & child) -> void
            {
                if (index == children.size())
                {
                    children.emplace_back(std::move(child));
                    return;
                }
                children.at(index) = std::move(child);
            }

            auto replace_at(size_t index, kv_pair && value) -> void
            {
                values.at(index) = std::move(value);
            }

            auto split_into(node & parent) -> void
            {
                auto mid = values.size() / 2;

                auto left = std::make_unique<node>();
                std::move(values.begin(), std::next(values.begin(), mid), std::back_inserter(left->values));
                std::move(children.begin(), std::next(children.begin(), mid+1), std::back_inserter(left->children));
                
                auto right = std::make_unique<node>();
                std::move(std::next(values.begin(), mid+1), values.end(), std::back_inserter(right->values));
                std::move(std::next(children.begin(), mid+1), children.end(), std::back_inserter(right->children));

                auto index = parent.insert_into(std::move(values[mid]));
                parent.set_child(index, left);
                parent.set_child(index+1, right);
            }

            auto find_child(Key const & key) -> node *
            {
                auto res = index_of(key);
                return children.at(res.index).get();
            }

            auto index_of(Key const & key) const -> index_of_result
            {
                auto it = values.lower_bound(key, [](auto const & v, auto const & key) { return v.first < key; });
                auto index = std::distance(values.begin(), it);
                if (it != values.end() and it->first == key)
                {
                    return {static_cast<size_t>(index), true};
                }
                return {static_cast<size_t>(index), false};
            }
            
            auto upper_index_of(Key const & key) const -> index_of_result
            {
                auto it = values.upper_bound(key, [](auto const & key, auto const & v) { return v.first >= key; });
                auto index = std::distance(values.begin(), it);
                if (it != values.end() and it->first == key)
                {
                    return {static_cast<size_t>(index), true};
                }
                return {static_cast<size_t>(index), false};
            }

            auto is_leaf() const -> bool
            {
                return children.empty() or children.front() == nullptr;
            }

            auto insert_into(kv_pair && data) -> size_t
            {
                auto res = upper_index_of(data.first);
                values.emplace_at(res.index, std::move(data));
                children.emplace_at(res.index, nullptr);
                return res.index;
            }
        };
            
        auto find(node * curr, Key const & key) -> index_of_result
        {
            while (curr)
            {
                auto it = curr->values.lower_bound(key, [](auto const & v, auto const & key) { return v.first < key; });
                auto index = std::distance(curr->values.begin(), it);
                if (it == curr->values.end() or it->first < key)
                {
                    curr = curr->children.at(index+1).get();
                }
                else if (it->first == key)
                {
                    return {curr, index};
                }
                else if (it->first > key)
                {
                    curr = curr->children.at(index).get();
                }
            }
            return {nullptr, 0};
        }

        auto print(node const & curr, std::string tab) const -> void
        {
            size_t index = 0;
            for (auto const & v : curr.values)
            {
                if (index < curr.children.size() and curr.children[index] != nullptr) {
                    print(*curr.children[index], tab + "  ");
                }
                std::cout << tab << "{" << v.first << ", " << v.second << "}\n";
                ++index;
            }
            if (index < curr.children.size() and curr.children[index] != nullptr) { print(*curr.children[index], tab + "  "); }
        }

        auto insert_into_leaf(node & curr, kv_pair & data) -> bool
        {
            if (not curr.is_leaf())
            {
                auto * next = curr.find_child(data.first);
                if (next == nullptr)
                {
                    throw std::runtime_error{"no child found"};
                }
                if (not insert_into_leaf(*next, data))
                {
                    next->split_into(curr);
                    return curr.values.size() < degree_;
                }
                return true;
            }
            
            curr.insert_into(std::move(data));

            return curr.values.size() < degree_;
        }

        auto remove_from(node & curr, Key const & key) -> remove_result
        {
            auto res = curr.index_of(key);
            if (not res.found)
            {
                if (curr.is_leaf())
                {
                    // Not found
                    return {false, false};
                }
            
                // Find the next level
                auto child_res = remove_from(*curr.children.at(res.index), key);
                if (child_res.rebalance)
                {
                    if (curr.values.size() != 0)
                    {
                        rebalance(curr, res.index);
                    }
                    else if (curr.children.at(res.index)->values.size() == 0)
                    {
                        curr.children.at(res.index) = std::move(curr.children.at(res.index)->children.at(0));
                    }
                }
                return child_res;
            }
  
            if (curr.is_leaf())
            {
                curr.remove_at(res.index);
                return {true, true};
            }
              
            curr.replace_at(res.index, steal_max(*curr.children.at(res.index)));
            rebalance(curr, res.index);
            return {true, true};
        }

        auto steal_max(node & n) -> kv_pair
        {
            if (n.is_leaf())
            {
                auto res = std::move(n.values.back());
                n.remove_at(n.values.size()-1);
                return res;
            }

            auto res = steal_max(*n.children.at(n.children.size()-1));
            rebalance(n, n.values.size()-1);
            return res;
        }

        auto rebalance(node & parent, size_t parent_value_index) -> bool
        {
            // parent should never need to be checked, just lchild and rchild
            auto lchild_index = parent_value_index;
            if (lchild_index == parent.children.size()-1)
            {
                lchild_index--;
            }
            auto & lchild = *parent.children.at(lchild_index);
            auto & rchild = *parent.children.at(lchild_index+1);
            auto borrow_from_lchild = lchild.values.size() > 1;
            auto borrow_from_rchild = rchild.values.size() > 1;
            if (underflows(lchild) and borrow_from_rchild)
            {
                // Rotate left
                rotate_left(parent, lchild_index, lchild, rchild);
                return true;
            }
            if (underflows(rchild) and borrow_from_lchild)
            {
                // Rotate right
                rotate_right(parent, lchild_index, lchild, rchild);
                return true;
            }
            if (underflows(lchild) or underflows(rchild))
            {
                // Merge
                merge(parent, lchild_index, lchild, rchild);
                return true;
            }

            return false;
        }

        auto rotate_left(node & parent, size_t parent_value_index, node & lchild, node & rchild) -> void
        {
            /*
                    (A)           (B)
                   /  \    ->     / \
                 ()    (B,C)    (A)  (C)

            */
            auto next_left_val = std::move(parent.values.at(parent_value_index));
            parent.remove_at(parent_value_index);

            auto next_parent_val = std::move(rchild.values.front());
            parent.values.emplace(parent.values.begin() + parent_value_index, next_parent_val);

            auto next_left_child = std::move(rchild.children.front());
            rchild.remove_at(0);
            rchild.remove_child_at(0);
            lchild.values.emplace_back(std::move(next_left_val));
            lchild.children.emplace_back(std::move(next_left_child));
        }

        auto rotate_right(node & parent, size_t parent_value_index, node & lchild, node & rchild) -> void
        {
            /*
                    (C)           (B)
                   /  \    ->     / \
                (A,B)  ()       (A)  (C)
            */
            auto next_right_val = std::move(parent.values.at(parent_value_index));
            parent.remove_at(parent_value_index);

            auto next_parent_val = std::move(lchild.values.back());
            parent.values.emplace(parent.values.begin() + parent_value_index, next_parent_val);

            auto next_right_child = std::move(lchild.children.back());
            auto old_idx = lchild.values.size();
            lchild.remove_at(old_idx-1);
            lchild.remove_child_at(old_idx);
            rchild.values.emplace(rchild.values.begin(), std::move(next_right_val));
            rchild.children.emplace(rchild.children.begin(), std::move(next_right_child));
        }

        auto merge(node & parent, size_t parent_value_index, node & lchild, node & rchild) -> void
        {
            /*
                    (A)          ()
                   /  \    ->    |
                 ()    (B)     (A,B)
            */
            auto new_index = lchild.insert_into(std::move(parent.values.at(parent_value_index)));
            // Remove empty child created by insert_into() -- new children added at the end
            lchild.remove_child_at(new_index);

            parent.remove_at(parent_value_index);

            for (auto & child : rchild.children)
            {
                lchild.children.emplace_back(std::move(child));
            }
            std::move(rchild.values.begin(), rchild.values.end(), std::back_inserter(lchild.values));
            rchild.values.clear();
            
            parent.remove_child_at(parent_value_index + 1);
        }

        auto underflows(node const & curr) const -> bool
        {
            return curr.values.size() < degree_-1;
        }

        std::unique_ptr<node> root_;
        size_t degree_;
    public:
        btree(size_t degree)
            : degree_{degree}
        {
        }

        auto find(Key const & key) const -> Value const *
        {
            if (root_)
            {
                return root_->find(key);
            }
            return nullptr;
        }

        auto remove(Key const & key) -> void
        {
            if (root_ == nullptr)
            {
                return;
            }

            remove_from(*root_, key);
            // check for 1 child, move up
            if (root_->children.size() == 1)
            {
                root_ = std::move(root_->children.at(0));
            }
        }

        auto insert(kv_pair && value) -> void
        {
            if (root_ == nullptr)
            {
                root_ = std::make_unique<node>();
            }
            if (not insert_into_leaf(*root_, value))
            {
                auto new_root = std::make_unique<node>();
                root_->split_into(*new_root);
                root_ = std::move(new_root);
            }
        }
        
        auto insert(Key const & key, Value && value) -> void
        {
            if (root_ == nullptr)
            {
                root_ = std::make_unique<node>();
            }
            auto data = std::make_pair(std::move(key), std::move(value));
            if (not insert_into_leaf(*root_, data))
            {
                auto new_root = std::make_unique<node>();
                root_->split_into(*new_root);
                root_ = std::move(new_root);
            }
        }

        auto print() const -> void
        {
            print(*root_, "");
        }
    };
}
