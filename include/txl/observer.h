#pragma once

#include <txl/type_info.h>

#include <functional>
#include <unordered_map>
#include <vector>

#include <algorithm>

#define LAZY_EMIT(m) ::txl::messaging::detail::global_dispatcher::instance().lazy_dispatch<decltype(m)>([&]() { return m; })
#define EMIT(m) ::txl::messaging::detail::global_dispatcher::instance().dispatch(m)
#define OBSERVE(o, msgs...) ::txl::messaging::detail::global_dispatcher::instance().add<msgs>(o)
#define UNOBSERVE(o) ::txl::messaging::detail::global_dispatcher::instance().remove(o)

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

    struct dispatcher
    {
    private:
        using callback_func = std::function<bool(void const *)>;
        struct callback final
        {
            void const * observer_;
            callback_func func_;

            callback(void const * observer, callback_func && func)
                : observer_{observer}
                , func_{std::move(func)}
            {
            }
        };

        using callback_list = std::vector<callback>;
        using type_to_callback_list = std::unordered_map<txl::type_info, callback_list>;
        using type_info_vector = std::vector<txl::type_info>;
        using observer_type_map = std::unordered_map<void const *, type_info_vector>;
        type_to_callback_list sub_map{};
        observer_type_map observer_to_types_{};

        template<class Msg, class Observer>
        auto subscribe_one(Observer & o) -> void
        {
            auto & type_list = observer_to_types_[&o];
            if (std::find(type_list.begin(), type_list.end(), txl::get_type_info<Msg>()) == type_list.end())
            {
                type_list.emplace_back(txl::get_type_info<Msg>());
            }

            auto & subs = sub_map[txl::get_type_info<Msg>()];
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
    public:
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
                auto & subs = sub_map[type_id];
                auto o_iter = std::find_if(subs.begin(), subs.end(), [&o](auto const & to_remove) {
                    return &o == to_remove.observer_;
                });
                if (o_iter != subs.end())
                {
                    subs.erase(o_iter);
                }
                if (subs.empty())
                {
                    sub_map.erase(type_id);
                }
            }
            observer_to_types_.erase(&o);
        }

        template<class... Msgs, class Observer>
        auto subscribe(Observer & o) -> void
        {
            subscribe_all<Msgs...>(o);
        }

        template<class Msg>
        auto dispatch(Msg const & msg)
        {
            auto subs = sub_map.find(txl::get_type_info<Msg>());
            if (subs == sub_map.end())
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
            auto subs = sub_map.find(txl::get_type_info<Msg>());
            if (subs == sub_map.end())
            {
                return;
            }

            if (subs->second.empty())
            {
                return;
            }

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
            dispatcher disp_;
        public:
            static auto instance() -> global_dispatcher &
            {
                static global_dispatcher g{};
                return g;
            }

            template<class Msg>
            auto dispatch(Msg const & msg) -> void
            {
                disp_.dispatch(msg);
            }

            template<class Msg, class MsgFactory>
            auto lazy_dispatch(MsgFactory const & msg_func) -> void
            {
                disp_.lazy_dispatch<Msg>(msg_func);
            }

            template<class... Msgs, class Observer>
            auto add(Observer & o) -> void
            {
                disp_.subscribe<Msgs...>(o);
            }

            template<class Observer>
            auto remove(Observer & o) -> void
            {
                disp_.unsubscribe_all(o);
            }
        };
    }
}
