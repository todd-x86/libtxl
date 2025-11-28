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

            auto left()
            {
                return children_.left_.get();
            }

            auto right()
            {
                return children_.right_.get();
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
