#pragma once

#include <txl/type_info.h>

#include <unordered_map>
#include <functional>

namespace txl
{
    class injector
    {
    public:
        using injector_factory = std::function<void*(injector &)>;

        template<class T>
        using injector_type_factory = std::function<T*(injector &)>;
    private:
        class injector_data final
        {
        private:
            injector_factory factory_;
            void * data_ = nullptr;

            injector_data(injector_factory factory)
                : factory_{factory}
            {
            }
        public:
            template<class T>
            static auto create(T & data) -> injector_data
            {
                return {[&data](auto &) { return &data; }};
            }

            template<class T>
            static auto create(injector_type_factory<T> data) -> injector_data
            {
                return {[data](auto & injector) { return data(injector); }};
            }

            template<class T>
            auto get(injector & i) -> T *
            {
                if (!data_)
                {
                    data_ = factory_(i);
                }
                return reinterpret_cast<T *>(data_);
            }
        };

        std::unordered_map<txl::type_info, injector_data> types_;
    public:
        template<class Base, class... OtherBases, class Derived>
        auto add(Derived & d) -> void
        {
            types_.emplace(txl::get_type_info<Base>(), injector_data::create(d));
            add<OtherBases...>(d);
        }

        template<class Derived>
        auto add(Derived & d) -> void
        {
            types_.emplace(txl::get_type_info<Derived>(), injector_data::create(d));
        }

        template<class T>
        auto get() -> T *
        {
            auto t = types_.find(txl::get_type_info<T>());
            if (t == types_.end())
            {
                return nullptr;
            }
            return t->second.template get<T>(*this);
        }

        template<class T>
        auto factory(injector_type_factory<T> f) -> void
        {
            types_.emplace(txl::get_type_info<T>(), injector_data::create(f));
        }
    };
}
