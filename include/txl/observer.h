#pragma once

#include <txl/lock_macros.h>
#include <txl/patterns.h>
#include <txl/polymorphic.h>
#include <txl/type_info.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#define LAZY_EMIT(m) ::txl::messaging::detail::global_dispatcher::instance().lazy_dispatch<decltype(m)>([&]() { return m; })
#define EMIT(m) ::txl::messaging::detail::global_dispatcher::instance().dispatch(m)
#define OBSERVE(o, msgs...) ::txl::messaging::detail::global_dispatcher::instance().add<msgs>(o)
#define UNOBSERVE(o, msgs...) ::txl::messaging::detail::global_dispatcher::instance().remove<msgs>(o)
#define UNOBSERVE_ALL(o) ::txl::messaging::detail::global_dispatcher::instance().remove_all(o)
#define CONNECT_OBSERVER(o) ::txl::messaging::detail::global_dispatcher::instance().connect(o)
#define DISCONNECT_OBSERVER(o) ::txl::messaging::detail::global_dispatcher::instance().disconnect(o)


namespace txl::messaging
{
    struct observer
    {
        template<class Msg>
        auto on_message(Msg const & msg) -> bool
        {
            return true;
        }
    };

    class dispatcher
    {
    private:
        struct callback final
        {
            using func_type = std::function<bool(void const *)>;

            void const * observer_;
            func_type func_;

            callback(void const * observer, func_type && func)
                : observer_{observer}
                , func_{std::move(func)}
            {
            }
        };

        using callback_list = std::vector<callback>;
        using type_to_callback_list = std::unordered_map<txl::type_info, callback_list>;
        using type_info_set = std::set<txl::type_info>;
        using observer_type_map = std::unordered_map<void const *, type_info_set>;

        type_to_callback_list sub_map_{};
        observer_type_map observer_to_types_{};

        template<class Msg, class Observer>
        auto subscribe_one(Observer & o) -> void
        {
            auto inserted = false;
            auto & type_list = observer_to_types_[&o];
            std::tie(std::ignore, inserted) = type_list.insert(txl::get_type_info<Msg>());

            if (not inserted)
            {
                // Already subscribed
                return;
            }

            auto & subs = sub_map_[txl::get_type_info<Msg>()];
            subs.emplace_back(static_cast<void const *>(&o), [o_ptr=&o](auto * msg_ptr) {
                return o_ptr->on_message(*static_cast<Msg const *>(msg_ptr));
            });
        }

        template<class Observer>
        auto subscribe_all(Observer & o) -> void
        {
            // Empty
        }
        
        template<class Msg, class... Msgs, class Observer>
        auto subscribe_all(Observer & o) -> void
        {
            subscribe_one<Msg>(o);
            subscribe_all<Msgs...>(o);
        }

        template<class Msg, class Observer>
        auto unsubscribe_one(Observer & o) -> void
        {
            auto observer_it = observer_to_types_.find(&o);
            if (observer_it == observer_to_types_.end())
            {
                return;
            }

            auto & type_id_set = *observer_it->second;
            if (auto it = type_id_set.find(txl::get_type_info<Msg>()); it != type_id_set.end())
            {
                // Remove from subscription map
                auto & subs = sub_map_[*it];
                auto o_iter = std::find_if(subs.begin(), subs.end(), [&o](auto const & to_remove) {
                    return &o == to_remove.observer_;
                });
                if (o_iter != subs.end())
                {
                    subs.erase(o_iter);
                }
                if (subs.empty())
                {
                    // Remove if no subscriptions to this type
                    sub_map_.erase(*it);
                }

                // Erase from observer map
                type_id_set.erase(it);
            }

            if (type_id_set.empty())
            {
                // Erase if observer is not subscribed to anything
                observer_to_types_.erase(observer_it);
            }
        }
        
        template<class Observer>
        auto unsubscribe_from(Observer & o) -> void
        {
            // Empty
        }
        
        template<class Msg, class... Msgs, class Observer>
        auto unsubscribe_from(Observer & o) -> void
        {
            unsubscribe_one<Msg>(o);
            unsubscribe_from<Msgs...>(o);
        }
    public:
        template<class... Msgs, class Observer>
        auto unsubscribe(Observer & o) -> void
        {
            unsubscribe_from<Msgs...>(o);
        }
        
        template<class Observer>
        auto unsubscribe_all(Observer & o) -> void
        {
            auto it = observer_to_types_.find(&o);
            if (it == observer_to_types_.end())
            {
                return;
            }
            for (auto type_id : it->second)
            {
                auto & subs = sub_map_[type_id];
                auto o_iter = std::find_if(subs.begin(), subs.end(), [&o](auto const & to_remove) {
                    return &o == to_remove.observer_;
                });
                if (o_iter != subs.end())
                {
                    subs.erase(o_iter);
                }
                if (subs.empty())
                {
                    sub_map_.erase(type_id);
                }
            }
            observer_to_types_.erase(it);
        }

        template<class... Msgs, class Observer>
        auto subscribe(Observer & o) -> void
        {
            subscribe_all<Msgs...>(o);
        }

        template<class Msg>
        auto dispatch(Msg const & msg)
        {
            auto subs = sub_map_.find(txl::get_type_info<Msg>());
            if (subs == sub_map_.end())
            {
                return;
            }

            auto it = subs->second.begin();
            while (it != subs->second.end())
            {
                if (it->func_(&msg))
                {
                    ++it;
                }
                else
                {
                    it = subs->second.erase(it);
                }
            }
        }

        template<class Msg, class Func>
        auto lazy_dispatch(Func const & msg_func)
        {
            auto subs = sub_map_.find(txl::get_type_info<Msg>());
            if (subs == sub_map_.end() or subs->second.empty())
            {
                return;
            }

            // Create the message (presumably expensive)
            auto msg = msg_func();

            auto it = subs->second.begin();
            while (it != subs->second.end())
            {
                if (it->func_(&msg))
                {
                    ++it;
                }
                else
                {
                    it = subs->second.erase(it);
                }
            }
        }
    };

    namespace detail
    {
        class global_dispatcher final
        {
        private:
            template<class T>
            using dispatcher_set = std::set<std::shared_ptr<T>>;

            ::txl::messaging::dispatcher disp_;
            ::txl::polymorphic_container_map<dispatcher_set> observers_;
            std::shared_mutex mutex_;
        public:
            static auto instance() -> global_dispatcher &
            {
                static global_dispatcher g{};
                return g;
            }

            template<class Observer>
            auto connect(std::shared_ptr<Observer> const & o) -> void
            {
                UNIQUE_LOCK(mutex_);

                observers_.get<Observer>().insert(o);
            }

            template<class Observer>
            auto disconnect(std::shared_ptr<Observer> const & o) -> void
            {
                UNIQUE_LOCK(mutex_);

                if (not observers_.contains<Observer>())
                {
                    return;
                }
                auto & observer_set = observers_.get<Observer>();
                if (auto it = observer_set.find(o); it != observer_set.end())
                {
                    observer_set.erase(it);
                }
            }

            template<class Msg>
            auto dispatch(Msg const & msg) -> void
            {
                SHARED_LOCK(mutex_);

                disp_.dispatch(msg);
            }

            template<class Msg, class MsgFactory>
            auto lazy_dispatch(MsgFactory const & msg_func) -> void
            {
                SHARED_LOCK(mutex_);

                disp_.lazy_dispatch<Msg>(msg_func);
            }

            template<class... Msgs, class Observer>
            auto add(Observer & o) -> void
            {
                UNIQUE_LOCK(mutex_);

                disp_.subscribe<Msgs...>(o);
            }
            
            template<class... Msgs, class Observer>
            auto remove(Observer & o) -> void
            {
                UNIQUE_LOCK(mutex_);

                disp_.unsubscribe<Msgs...>(o);
            }

            template<class Observer>
            auto remove_all(Observer & o) -> void
            {
                UNIQUE_LOCK(mutex_);

                disp_.unsubscribe_all(o);
            }
            
            template<class... Msgs, class Observer>
            auto add(std::shared_ptr<Observer> & o) -> void
            {
                connect(o);
                add<Msgs...>(*o);
            }
            
            template<class... Msgs, class Observer>
            auto remove(std::shared_ptr<Observer> const & o) -> void
            {
                remove<Msgs...>(*o);
            }

            template<class Observer>
            auto remove_all(std::shared_ptr<Observer> & o) -> void
            {
                remove_all(*o);
                disconnect(o);
            }
        };
    }
}
