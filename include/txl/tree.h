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

            auto assign(node_ptr & n)
            {
                // Move n away temporarily so it destructs on its own here
                auto local = std::move(n);
                data_ = std::move(local->data_);
                children_ = std::move(local->children_);
            }

            auto min_value() -> kv_pair
            {
                node * parent = this;
                node * n = right();
                while (n and n->left())
                {
                    parent = n;
                    n = n->left();
                }

                if (parent == this)
                {
                    auto res = std::move(n->data_);
                    // Handle right child and promote up
                    parent->right_ptr() = std::move(n->right_ptr());
                    return res;
                }
                auto removed = std::move(parent->left_ptr());
                // Handle right child and promote up
                parent->left_ptr() = std::move(removed->right_ptr());
                return std::move(removed->data_);
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
            if (root_)
            {
                print(*root_, "");
            }
        }

        auto remove(Key const & key) -> void
        {
            node * parent = nullptr;
            auto n = root_.get();
            auto cmp = Less<Key>();
            auto left_child = false;
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
                    left_child = true;
                    n = n->left();
                }
                else
                {
                    parent = n;
                    left_child = false;
                    n = n->right();
                }
            }

            if (n == nullptr)
            {
                // Not found
                return;
            }

            // Root node case
            if (parent == nullptr)
            {
                if (root_->left() and root_->right())
                {
                    root_->data_ = root_->min_value();
                }
                else if (root_->right())
                {
                    root_ = std::move(root_->right_ptr());
                }
                else if (root_->left())
                {
                    root_ = std::move(root_->left_ptr());
                }
                else
                {
                    root_.reset();
                }
                return;
            }

            // If leaf node, just remove it
            if (n->is_leaf())
            {
                if (left_child)
                {
                    parent->left_ptr().reset();
                }
                else
                {
                    parent->right_ptr().reset();
                }
                return;
            }

            // Remove from middle
            if (n->left() and n->right())
            {
                n->data_ = n->min_value();
            }
            else if (n->right())
            {
                n->assign(n->right_ptr());
            }
            else
            {
                n->assign(n->left_ptr());
            }
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
