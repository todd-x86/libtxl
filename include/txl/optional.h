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
}
