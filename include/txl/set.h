#pragma once

#include <txl/patterns.h>

#include <initializer_list>
#include <vector>

namespace txl
{
    template<class Value, class Less = std::less<Value>, class Container = std::vector<Value>>
    class set
    {
    private:
        Container values_;
        
        template<class Key, class CmpLess, class KeyExtractor>
        struct key_comparer final
        {
            KeyExtractor key_extractor;

            auto operator()(Key const & key1, Key const & key2) const -> bool
            {
                return CmpLess{}(key1, key2);
            }
            
            auto operator()(Key const & key, Value const & value) const -> bool
            {
                return (*this)(key, key_extractor(value));
            }

            auto operator()(Value const & value, Key const & key) const -> bool
            {
                return (*this)(key_extractor(value), key);
            }
            
            auto operator()(Value const & value1, Value const & value2) const -> bool
            {
                return (*this)(key_extractor(value1), key_extractor(value2));
            }
        };
    public:
        using const_iterator = typename Container::const_iterator;
        using value_type = typename Container::value_type;

        set() = default;
        set(std::initializer_list<Value> values)
        {
            for (auto const & val : values)
            {
                add(val);
            }
        }

        auto contains(Value const & v) const -> bool
        {
            auto it = lower_bound(v);
            auto cmp = Less{};
            return (it != end() and not cmp(*it, v) and not cmp(v, *it));
        }
        
        auto lower_bound(Value const & v) const -> const_iterator
        {
            return txl::lower_bound(values_, v, Less{});
        }

        auto begin() const -> const_iterator
        {
            return values_.begin();
        }

        auto end() const -> const_iterator
        {
            return values_.end();
        }
        
        auto add(Value const & v)
        {
            auto cmp = Less{};
            auto it = txl::lower_bound(values_, v, cmp);

            if (it != end() and not cmp(*it, v) and not cmp(v, *it))
            {
                // Already exists
                return;
            }
            values_.emplace(it, v);
        }

        auto add(Value && v)
        {
            auto cmp = Less{};
            auto it = txl::lower_bound(values_, v, cmp);

            if (it != end() and not cmp(*it, v) and not cmp(v, *it))
            {
                // Already exists
                return;
            }
            values_.emplace(it, std::move(v));
        }

        auto erase(Value const & v) -> void
        {
            if (auto it = lower_bound(v); it != end())
            {
                erase(it);
            }
        }

        auto erase(const_iterator it) -> void
        {
            values_.erase(it);
        }

        auto size() const -> size_t
        {
            return values_.size();
        }

        template<class Key, class KeyLess, class C, class KeyExtractFunc>
        auto intersect(set<Key, KeyLess, C> const & keys, KeyExtractFunc key_extractor)
        {
            set res{};
            std::set_intersection(begin(), end(),
                                  keys.begin(), keys.end(),
                                  std::back_inserter(res.values_),
                                  key_comparer<Key, KeyLess, KeyExtractFunc>{key_extractor});
            return res;
        }
        
        auto intersect(set const & values)
        {
            set res{};
            std::set_intersection(values.begin(), values.end(),
                                  begin(), end(),
                                  std::back_inserter(res.values_),
                                  Less{});
            return res;
        }

        auto merge(set const & values)
        {
            set res{};
            std::set_union(values.begin(), values.end(),
                           begin(), end(),
                           std::back_inserter(res.values_),
                           Less{});
            return res;
        }

        auto merge_from(set && values)
        {
            auto x = begin();
            auto y = values.begin();

            auto cmp = Less{};

            while (x != end() and y != values.end())
            {
                while (x != end() and cmp(*x, *y))
                {
                    ++x;
                }

                x = values_.emplace(x, std::move(*y));
                ++y;
                ++x;
            }
            while (y != values.end())
            {
                values_.emplace(end(), std::move(*y));
                ++y;
            }
        }
    };
}
