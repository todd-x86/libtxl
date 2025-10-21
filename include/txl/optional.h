#pragma once

namespace txl
{
    template<class T>
    struct optional : std::optional<T>
    {
        template<class FactoryFunc>
        auto value_or_lazy(FactoryFunc && func) const -> T
        {
            if (this->has_value())
            {
                return *this;
            }
            return func();
        }
        
        template<class FactoryFunc>
        auto value_or_lazy(FactoryFunc && func) -> T &&
        {
            if (this->has_value())
            {
                return std::move(*this);
            }
            return std::move(func());
        }
    };

    template<class T>
    class optional_ref
    {
    private:
        T * data_;
    public:
        template<class Opt>
        optional_ref(std::optional<Opt> & data)
            : data_{data.has_value() ? &(*data) : nullptr}
        {
        }
        
        template<class Opt>
        optional_ref(std::optional<Opt> const & data)
            : data_{data.has_value() ? const_cast<Opt *>(&(*data)) : nullptr}
        {
        }

        auto operator->() -> T *
        {
            return data_;
        }

        auto operator*() -> T &
        {
            return *data_;
        }

        auto operator->() const -> T const *
        {
            return data_;
        }

        auto operator*() const -> T const &
        {
            return *data_;
        }
    };
}
