#pragma once

#include <cstddef>

namespace txl
{
    struct size_policy
    {
        size_t value;

        size_policy(size_t i)
            : value(i)
        {
        }

        auto process(size_t requested, size_t actual) -> void
        {
            if (actual >= value)
            {
                value = 0;
            }
            else
            {
                value -= actual;
            }
        }

        auto is_complete() const -> bool
        {
            return value == 0;
        }
    };

    struct exactly final : size_policy
    {
    };
    
    struct at_most final : size_policy
    {
        bool maybe_eof_ = false;

        auto process(size_t requested, size_t actual) -> void
        {
            maybe_eof_ = (requested > actual);
            size_policy::process(requested, actual);
        }

        auto is_complete() const -> bool
        {
            return value == 0 or maybe_eof_;
        }
    };

    template<class... SizePolicies>
    class one_of : public size_policy
    {
    private:
        std::tuple<SizePolicies...> size_policies_;

        template<size_t Index>
        auto process_policies(size_t requested, size_t actual) -> void
        {
            std::get<Index>(size_policies_).process(requested, actual);
            if constexpr (Index > 0)
            {
                process_policies<Index-1>(requested, actual);
            }
        }
        
        template<size_t Index>
        auto is_policy_complete() const -> bool
        {
            if (std::get<Index>(size_policies_).is_complete())
            {
                return true;
            }

            if constexpr (Index > 0)
            {
                return is_policy_complete<Index-1>();
            }

            return false;
        }
    public:
        one_of(SizePolicies && ... size_policies)
            : size_policies_{std::forward<SizePolicies>(size_policies)...}
        {
        }

        auto process(size_t requested, size_t actual) -> void
        {
            process_policies<std::tuple_size_v<SizePolicies...>-1>(requested, actual);
        }

        auto is_complete() const -> bool
        {
            return is_policy_complete<std::tuple_size_v<SizePolicies...>-1>();
        }
    };
}
