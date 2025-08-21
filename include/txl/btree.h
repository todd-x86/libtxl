#pragma once

#include <txl/storage.h>

#include <vector>

namespace txl
{
    template<class Key, class Value>
    class btree final
    {
    private:
        struct node final
        {
            size_t max_values;
            std::vector<storage<std::pair<Key, Value>>> entries;
            std::vector<node *> children;

            node(size_t size)
                : max_values{size}
            {
            }

            ~node()
            {
                // TODO
            }

            auto insert(Key && key, Value && value) -> bool
            {
                if (entries.size() 
            }
        };
        
        size_t degree_;
        node * root_ = nullptr;
    public:
        btree(size_t degree)
            : degree_{degree}
        {
        }

        ~btree()
        {
            delete root_;
        }

        auto insert(Key && key, Value && value) -> void
        {
        }
    };
}
