#pragma once

#include <memory>
#include <utility>

namespace txl
{
    template<class Key, class Value, template<class> class Less = std::less>
    class binary_search_tree final
    {
    public:
        using kv_pair = std::pair<Key, Value>;
    private:
        struct node;
        using node_ptr = std::unique_ptr<node>;

        struct node_pair final
        {
            node_ptr left_;
            node_ptr right_;
        };

        struct node final
        {
            kv_pair data_;
            node_pair children_;

            node(kv_pair && data)
                : data_{std::move(data)}
                , children_{nullptr, nullptr}
            {
            }
            
            auto min_value() -> std::optional<kv_pair>
            {
                node * parent = this;
                node * n = left();
                while (n)
                {
                    parent = n;
                    n = n->left();
                }
                if (not n)
                {
                    std::cout << "SELF\n";
                    return {std::move(data_)};
                }
                if (n->right())
                {
                    // Promote up
                    auto res = std::move(n->data_);
                    parent->children_.left_ = std::move(n->right_ptr());
                    return {std::move(res)};
                }

                auto removed = std::move(parent->left_ptr());
                return {std::move(removed->data_)};
            }
            
            auto max_value() -> std::optional<kv_pair>
            {
                node * parent = this;
                node * n = right();
                while (n)
                {
                    parent = n;
                    n = n->right();
                }
                if (not n)
                {
                    std::cout << "SELF\n";
                    return {std::move(data_)};
                }
                if (n->left())
                {
                    // Promote up
                    auto res = std::move(n->data_);
                    parent->children_.right_ = std::move(n->left_ptr());
                    return {std::move(res)};
                }

                auto removed = std::move(parent->right_ptr());
                return {std::move(removed->data_)};
            }

            auto left()
            {
                return children_.left_.get();
            }

            auto right()
            {
                return children_.right_.get();
            }
            
            auto left_ptr() -> node_ptr &
            {
                return children_.left_;
            }

            auto right_ptr() -> node_ptr &
            {
                return children_.right_;
            }
            
            auto left() const
            {
                return children_.left_.get();
            }

            auto right() const
            {
                return children_.right_.get();
            }

            auto is_leaf() const
            {
                return children_.left_ == nullptr and children_.right_ == nullptr;
            }
        };

        auto print(node const & n, std::string tab) const -> void {
            if (n.left()) {
                print(*n.left(), tab + "\t");
            }
            std::cout << tab << "{" << n.data_.first << "," << n.data_.second << "}\n";
            if (n.right()) {
                print(*n.right(), tab + "\t");
            }
        }

        node_ptr root_;
    public:
        auto find(Key const & key) const -> kv_pair const *
        {
            auto n = root_.get();
            auto cmp = Less<Key>();
            while (n)
            {
                auto key_lt = cmp(key, n->data_.first);
                auto key_gt = cmp(n->data_.first, key);
                if (not key_lt and not key_gt)
                {
                    return &n->data_;
                }

                if (key_lt)
                {
                    n = n->left();
                }
                else
                {
                    n = n->right();
                }
            }
            return nullptr;
        }

        auto print() const -> void {
            print(*root_, "");
        }

        auto remove(Key const & key) -> void
        {
            node * parent = nullptr;
            auto n = root_.get();
            auto cmp = Less<Key>();
            while (n)
            {
                auto key_lt = cmp(key, n->data_.first);
                auto key_gt = cmp(n->data_.first, key);
                if (not key_lt and not key_gt)
                {
                    break;
                }

                if (key_lt)
                {
                    parent = n;
                    n = n->left();
                }
                else
                {
                    parent = n;
                    n = n->right();
                }
            }

            if (n == nullptr)
            {
                // Not found
                return;
            }

            std::optional<kv_pair> stolen;
            if (n->left())
            {
                // Pull max from left
                stolen = n->max_value();
            }
            else if (n->right())
            {
                // Pull min from right
                stolen = n->min_value();
            }

            if (stolen)
            {
                // Move up max or min value to replace it
                std::cout << "STOLEN\n";
                n->data_ = std::move(*stolen);
                return;
            }

            if (parent == nullptr)
            {
                // Deleting root
                std::cout << "ROOT " << std::boolalpha << (n->left() != nullptr) << "|" << (n->right() != nullptr) << "\n";
                root_.reset();
                return;
            }

            // Delete leaf node
            if (parent->left() == n)
            {
                parent->left_ptr().reset();
                std::cout << "DELETE1\n";
                return;
            }
            parent->right_ptr().reset();
            std::cout << "DELETE2\n";
        }

        auto emplace(Key && key, Value && value) -> void
        {
            auto data = std::make_pair(std::move(key), std::move(value));
            if (root_ == nullptr)
            {
                root_ = std::make_unique<node>(std::move(data));
                return;
            }

            auto cmp = Less<Key>{};

            auto * n = root_.get();
            while (n)
            {
                auto key_lt = cmp(data.first, n->data_.first);
                if (key_lt and n->left())
                {
                    n = n->left();
                }
                else if (not key_lt and n->right())
                {
                    n = n->right();
                }
                else
                {
                    break;
                }
            }
            if (not n)
            {
                return;
            }
            if (cmp(data.first, n->data_.first))
            {
                n->children_.left_ = std::make_unique<node>(std::move(data));
            }
            else
            {
                n->children_.right_ = std::make_unique<node>(std::move(data));
            }
        }
    };
}
