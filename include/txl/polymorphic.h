#pragma once

#include <txl/type_info.h>

#include <functional>
#include <unordered_map>

namespace txl
{
    class polymorphic_map
    {
    protected:
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

        template<class T>
        static inline auto get_deleter()
        {
            return [](auto * p) { delete static_cast<T *>(p); };
        }
    protected:
        template<class T, class... CtorArgs>
        auto find_or_create(type_info const & ti, CtorArgs && ... ctor_args) -> void *
        {
            auto it = type_map_.find(ti);
            if (it == type_map_.end())
            {
                auto emplaced = false;
                std::tie(it, emplaced) = type_map_.emplace(ti, container{get_deleter<T>()});
                if (emplaced)
                {
                    it->second.assign(new T(std::forward<CtorArgs>(ctor_args)...));
                }
            }
            return it->second.get();
        }
    public:
        auto clear() -> void
        {
            type_map_.clear();
        }

        auto contains(type_info const & ti) const -> bool { return type_map_.find(ti) != type_map_.end(); }

        template<class T>
        auto contains() const -> bool { return contains(get_type_info<T>()); }

        auto size() const -> size_t { return type_map_.size(); }

        template<class T, class... CtorArgs>
        auto get(CtorArgs && ... ctor_args) -> T &
        {
            auto * c = find_or_create<T>(get_type_info<T>(), std::forward<CtorArgs>(ctor_args)...);
            return *static_cast<T *>(c);
        }

        template<class T>
        auto get() const -> T const *
        {
            if (auto it = type_map_.find(get_type_info<T>()); it != type_map_.end())
            {
                return static_cast<T const *>(it->second.get());
            }
            return nullptr;
        }
    };

    /**
     * Unordered map of a `type_info` to a container of like objects.  This is a "container of containers" where
     * each key-value pair represents a container for a specific type.  All containers under the `polymorphic_map`
     * conform to the same type specified (`Container`).
     *
     * \tparam Container container type accepting a single type for template instantiation
     */
    template<template<class> class Container>
    struct polymorphic_container_map : polymorphic_map
    {
        template<class T>
        auto contains() const -> bool { return polymorphic_map::contains(get_type_info<Container<T>>()); }

        template<class T, class... CtorArgs>
        auto get(CtorArgs && ... ctor_args) -> Container<T> &
        {
            return polymorphic_map::get<Container<T>, CtorArgs...>(std::forward<CtorArgs>(ctor_args)...);
        }

        template<class T>
        auto get() const -> Container<T> const *
        {
            return polymorphic_map::get<Container<T>>();
        }
    };
}
