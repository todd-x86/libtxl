#pragma once

#include <txl/type_info.h>

#include <functional>
#include <unordered_map>

namespace txl
{
    template<template<class> class Container>
    class polymorphic_map
    {
    private:
        struct container
        {
            void * data_;
            std::function<void(void *)> deleter_;
        };

        std::unordered_map<type_info, container> type_map_;

        template<class T>
        auto find_or_create() -> void *
        {
            auto t = get_type_info<T>();
            if (auto it = type_map_.find(t); it != type_map_.end())
            {
                return it->second.data_;
            }
            // TODO: strong exception guarantee here (assign after emplace)
            auto * p = new Container<T>();
            type_map_.emplace(t, container{p, [](auto * p) { delete static_cast<Container<T> *>(p); }});
            return p;
        }
    public:
        polymorphic_map() = default;

        virtual ~polymorphic_map()
        {
            clear();
        }

        auto clear() -> void
        {
            for (auto [_, p] : type_map_)
            {
                p.deleter_(p.data_);
            }
            type_map_.clear();
        }

        template<class T>
        auto get() -> Container<T> &
        {
            auto * c = find_or_create<T>();
            return *static_cast<Container<T> *>(c);
        }

        template<class T>
        auto get() const -> Container<T> const *
        {
            auto t = get_type_info<T>();
            void * c = nullptr;
            if (auto it = type_map_.find(t); it != type_map_.end())
            {
                c = it->second.data_;
            }
            return static_cast<Container<T> *>(c);
        }
    };
}
