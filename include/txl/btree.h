#pragma once

#include <txl/storage.h>

#include <memory>
#include <optional>
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
            std::vector<node_ptr> children;
            std::vector<kv_pair> values;

            node()
            {
                //values.reserve(10);
            }

            auto print_children() const -> void
            {
		std::cout << "{" << std::endl;
                for (auto const & c : children)
		{
			if (c)
			{
                    std::cout << " - " << c->values[0].first << ".." << c->values[c->values.size()-1].first << std::endl;
			}
			else
			{
                    std::cout << " - null" << std::endl;
			}
                }
		std::cout << "}" << std::endl;
            }

            auto num_children() const -> size_t
            {
                return std::count_if(children.begin(), children.end(), [](auto const & c) { return c != nullptr; });
            }
            
            auto free_child_index() const -> size_t
            {
                auto it = std::find_if(children.begin(), children.end(), [](auto const & c) { return c != nullptr; });
                if (it == children.end())
                {
                    throw std::runtime_error{"no free child"};
                }
                return std::distance(children.begin(), it);
            }

            auto child(size_t index) -> node &
            {
                return *children.at(index);
            }
            
            auto replace_at(size_t index, node && n) -> void
            {
                children.at(index) = std::move(n);
            }

            auto release_max() -> kv_pair
            {
                auto n = this;
                while (not n->is_leaf())
                {
                    n = n->children.back().get();
                }
                auto last = std::move(n->values.back());
                n->remove_child_at(n->values.size());
                n->values.erase(n->values.begin() + n->values.size()-1);
                return last;
            }

            auto remove_at(size_t index) -> void
            {
                values.erase(values.begin() + index);
            }

            auto remove_child_at(size_t index) -> void
            {
                children.erase(children.begin() + index);
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

            auto replace_at(size_t index, kv_pair && value) -> void
            {
                std::cout << "replace " << values.at(index).first << " with " << value.first << "\n";
                values.at(index) = std::move(value);
            }

            auto get_successor(size_t index) -> kv_pair
            {
                //auto const & key = values.at(index).first;
                std::cout << "get_successor()\n";
                auto const & child = children.at(index);
                return child->release_max();
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

            auto index_of(Key const & key) -> find_result
            {
                auto it = std::lower_bound(values.begin(), values.end(), key, [](auto const & v, auto const & key) { return v.first < key; });
                auto index = std::distance(values.begin(), it);
                if (it == values.end() or it->first < key)
                {
                    return {static_cast<size_t>(index), false};
                }
                else if (it->first == key)
                {
                    return {static_cast<size_t>(index), true};
                }
                else if (it->first > key)
                {
                    return {static_cast<size_t>(index), false};
                }
                return {static_cast<size_t>(index), false};
            }

            auto left_child(size_t index) -> node &
            {
                return *children.at(index);
            }

            auto right_child(size_t index) -> node &
            {
                return *children.at(index+1);
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
                    return curr.num_values() < degree_;
                }
                return true;
            }
            
            curr.insert_into(std::move(data));

            return curr.num_values() < degree_;
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
                auto child_res = remove_from(curr.child(res.index), key);
                if (child_res.rebalance)
                {
                    if (curr.values.size() != 0)
                    {
                        rebalance(curr, res.index);
                    }
                    else if (curr.child(res.index).values.size() == 0)
                    {
                        std::cout << "PROMOTE!\n";
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
              
            curr.replace_at(res.index, steal_max(curr.child(res.index)));
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

            auto res = steal_max(n.child(n.children.size()-1));
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
            auto & lchild = parent.child(lchild_index);
            auto & rchild = parent.child(lchild_index+1);
            auto lchild_underflows = (lchild.values.size() < 1);
            auto rchild_underflows = (rchild.values.size() < 1);
            auto borrow_from_lchild = lchild.values.size() > 1;
            auto borrow_from_rchild = rchild.values.size() > 1;
            if (lchild_underflows and borrow_from_rchild)
            {
                // Rotate left
                rotate_left(parent, lchild_index, lchild, rchild);
                return true;
            }
            if (rchild_underflows and borrow_from_lchild)
            {
                // Rotate right
                rotate_right(parent, lchild_index, lchild, rchild);
                return true;
            }
            if (lchild_underflows or rchild_underflows)
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
            std::cout << "ROTATE LEFT (lc=" << lchild.children.size() << ", rc=" << rchild.children.size() << ")\n";
            lchild.insert_into(std::move(parent.values.front()));
            parent.remove_at(0);
            parent.insert_into(std::move(rchild.values.front()));
            rchild.remove_at(0);
        }

        auto rotate_right(node & parent, size_t parent_value_index, node & lchild, node & rchild) -> void
        {
            /*
                    (C)           (B)
                   /  \    ->     / \
                (A,B)  ()       (A)  (C)
            */
            std::cout << "ROTATE RIGHT (lc=" << lchild.children.size() << ", rc=" << rchild.children.size() << ")\n";
            rchild.insert_into(std::move(parent.values.back()));
            parent.remove_at(parent.values.size()-1);
            parent.insert_into(std::move(lchild.values.back()));
            lchild.remove_at(lchild.values.size()-1);
        }

        auto merge(node & parent, size_t parent_value_index, node & lchild, node & rchild) -> void
        {
            /*
                    (A)          ()
                   /  \    ->    |
                 ()    (B)     (A,B)
            */
            print();

            std::cout << "MERGE\n";
            std::cout << " -- [0," << parent_value_index << "," << parent.values.size()-1 << "]=values, [0," << parent_value_index << "," << parent.children.size()-1 << "]=children\n";
            //std::cout << "{" << parent.values.at(parent_value_index).first << " (parent)";
            for (auto const & kv : lchild.values)
            {
                std::cout << ", " << kv.first << " (left)";
            }
            for (auto const & kv : rchild.values)
            {
                std::cout << ", " << kv.first << " (right)";
            }
            std::cout << "}\n";

            lchild.print_children();
            auto new_index = lchild.insert_into(std::move(parent.values.at(parent_value_index == parent.values.size() ? parent_value_index-1 : parent_value_index)));
	    lchild.remove_child_at(new_index);
	    // TODO: why off by 1?
	    lchild.children.emplace(lchild.children.begin() + new_index + 1, nullptr);
            lchild.print_children();
            parent.remove_at(parent_value_index);

            std::cout << "TODO: lchild.children=" << lchild.num_children() << ", rchild.children=" << rchild.num_children() << "\n";
            auto rchildren = rchild.num_children();
            for (size_t i = 0; i < rchildren; ++i)
            {
                std::cout << "RCHILD[" << i << "]: " << rchild.children.at(i)->values.at(0).first << "\n";
                auto insert_index = lchild.num_children();
		if (insert_index < lchild.children.size())
		{
			lchild.children.at(lchild.num_children()) = std::move(rchild.children.at(i));
		}
		else
		{
			lchild.children.emplace_back(std::move(rchild.children.at(i)));
		}
		std::cout << "---------" << std::endl;
            }
            std::move(rchild.values.begin(), rchild.values.end(), std::back_inserter(lchild.values));
            rchild.values.clear();
            
            parent.remove_child_at(parent_value_index + 1);

            /*if (parent_value_index < parent.values.size())
            {
                lchild.insert_into(std::move(parent.values.at(parent_value_index)));
            }
            parent.remove_at(parent_value_index);
            std::move(rchild.values.begin(), rchild.values.end(), std::back_inserter(lchild.values));
            std::move(rchild.children.begin(), rchild.children.end(), std::back_inserter(lchild.children));
            //rchild.values.clear();
            // TODO: is this rchild?
            parent.children.erase(parent.children.begin() + parent_value_index + 1);
            */
        }


        auto merge_old(node & parent, node & lchild, node & rchild) -> node
        {
            /*
                    (A)          ()        (A,B)
                   /  \    ->    |    ->   
                 ()    (B)     (A,B)

                  remove()     (merge)      (lift)


                       (C)              (C)                ()             (C,D)
                      /   \            /   \               |              / | \
                    (A)   (D)   ->   ()     (D)    ->    (C,D)     -> (A,B)(E)(F) 
                    / \   / \        |      / \          / | \
                  ()  (B)(E)(F)     (A,B) (E) (F)    (A,B)(E)(F)
                  
                    remove()           merge()          merge()         lift()

             */
            node res{};
            std::move(lchild.values.begin(), lchild.values.end(), std::back_inserter(res.values));
            std::move(parent.values.begin(), parent.values.end(), std::back_inserter(res.values));
            std::move(rchild.values.begin(), rchild.values.end(), std::back_inserter(res.values));
            std::move(lchild.children.begin(), lchild.children.end(), std::back_inserter(res.children));
            std::move(rchild.children.begin(), rchild.children.end(), std::back_inserter(res.children));
            return res;
        }

        auto underflows(node const & curr) const -> bool
        {
            // FIXME: use proper degree
            return curr.values.size() < 1;
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
            std::cout << "REMOVE(" << key << ")\n";
            remove_from(*root_, key);
            // TODO: check for 1 child, move up
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
        }

        auto print() const -> void
        {
            std::cout << "v TREE-------------------\n";
            print(*root_, "");
            std::cout << "^ TREE-------------------\n";
        }
    };
}
