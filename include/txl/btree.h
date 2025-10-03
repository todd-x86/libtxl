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

        using node_ptr = std::unique_ptr<node>;

        struct find_result final
        {
            node * n;
            size_t index;
        };

        struct node final
        {
            std::vector<node_ptr> children;
            std::vector<kv_pair> values;

            node()
            {
                //values.reserve(10);
            }

            auto num_values() const -> size_t
            {
                return values.size();
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
                auto it = std::lower_bound(values.begin(), values.end(), key, [](auto const & v, auto const & key) { return v.first < key; });
                if (it == values.end())
                {
                    if (children.size() == 0)
                    {
                        return nullptr;
                    }
                    return children.back().get();
                }
                if (it->first == key)
                {
                    return this;
                }
                auto index = std::distance(values.begin(), it);
                if (it->first < key)
                {
                    return children.at(index+1).get();
                }
                return children.at(index).get();
            }

            auto is_leaf() const -> bool
            {
                return std::find_if(children.begin(), children.end(), [](auto const & node) { return node != nullptr; }) == children.end();
            }

            auto insert_into(kv_pair && data) -> size_t
            {
                auto it = std::lower_bound(values.begin(), values.end(), data, [](auto const & x, auto const & y) {
                    return x.first < y.first;
                });
                auto index = std::distance(values.begin(), it);
                values.emplace(it, std::move(data));
                children.emplace(std::next(children.begin(), index), nullptr);
                return index;
            }
        };
            
        auto find(node * curr, Key const & key) -> find_result
        {
            while (curr)
            {
                auto it = std::lower_bound(curr->values.begin(), curr->values.end(), key, [](auto const & v, auto const & key) { return v.first < key; });
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
            auto index = 0;
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
                    return curr.num_values() < degree_;
                }
                return true;
            }
            
            curr.insert_into(std::move(data));

            return curr.num_values() < degree_;
        }

        auto remove_from(node & parent, Key const & key) -> bool
        {
            auto res = parent.find(key);
            if (n == 
        }

        std::unique_ptr<node> root_;
        size_t degree_;
    public:
        btree(size_t degree)
            : degree_{degree}
        {
        }

        auto remove(Key const & key) -> void
        {
            remove_from(*root_, key);
        }

        auto insert(Key && key, Value && value) -> void
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
            std::cout << "v TREE-------------------\n";
            print(*root_, "");
            std::cout << "^ TREE-------------------\n";
        }
    };
}
