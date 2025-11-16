#pragma once

#include <txl/type_info.h>

#include <functional>
#include <unordered_map>

namespace txl
{
    /**
     * Unordered map of a `type_info` to a container of like objects.  This is a "container of containers" where
     * each key-value pair represents a container for a specific type.  All containers under the `polymorphic_map`
     * conform to the same type specified (`Container`).
     *
     * \tparam Container container type accepting a single type for template instantiation
     */
    template<template<class> class Container>
    class polymorphic_map
    {
    private:
        /**
         * Container wrapper which owns the lifetime of the container.
         */
        class container final
        {
        private:
            void * data_;
            std::function<void(void *)> deleter_;
        public:
            container(std::function<void(void *)> && deleter)
                : data_{nullptr}
                , deleter_{std::move(deleter)}
            {
            }

            ~container()
            {
                if (data_ and deleter_) { deleter_(data_); }
            }

            auto assign(void * data)
            {
                data_ = data;
            }

            auto get()
            {
                return data_;
            }
        };

        std::unordered_map<type_info, container> type_map_;

        template<class T, class... CtorArgs>
        auto find_or_create(CtorArgs && ... ctor_args) -> void *
        {
            auto t = get_type_info<T>();
            auto it = type_map_.find(t);
            if (it == type_map_.end())
            {
                auto emplaced = false;
                std::tie(it, emplaced) = type_map_.emplace(t, container{[](auto * p) { delete static_cast<Container<T> *>(p); }});
                if (emplaced)
                {
                    it->second.assign(new Container<T>(std::forward<CtorArgs>(ctor_args)...));
                }
            }
            return it->second.get();
        }
    public:
        auto clear() -> void
        {
            type_map_.clear();
        }

        auto size() const -> size_t { return type_map_.size(); }

        template<class T, class... CtorArgs>
        auto get(CtorArgs && ... ctor_args) -> Container<T> &
        {
            auto * c = find_or_create<T>(std::forward<CtorArgs>(ctor_args)...);
            return *static_cast<Container<T> *>(c);
        }

        template<class T>
        auto get() const -> Container<T> const *
        {
            void * c = nullptr;
            if (auto it = type_map_.find(get_type_info<T>()); it != type_map_.end())
            {
                c = it->second.get();
            }
            return static_cast<Container<T> const *>(c);
        }
    };
}
