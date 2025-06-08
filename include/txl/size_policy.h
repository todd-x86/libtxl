#pragma once

#include <txl/buffer_ref.h>
#include <algorithm>
#include <cstddef>

namespace txl
{
    class size_policy
    {
    private:
        size_t value_;
    public:
        size_policy(size_t i)
            : value_{i}
        {
        }

        auto value() const -> size_t
        {
            return value_;
        }

        auto process(buffer_ref requested, buffer_ref actual) -> void
        {
            if (actual.size() >= value_)
            {
                value_ = 0;
            }
            else
            {
                value_ -= actual.size();
            }
        }

        auto is_complete() const -> bool
        {
            return value_ == 0;
        }
    };

    struct exactly final : size_policy
    {
    };
    
    struct at_most final : size_policy
    {
        bool maybe_eof_ = false;

        auto process(buffer_ref requested, buffer_ref actual) -> void
        {
            maybe_eof_ = (requested.size() > actual.size());
            size_policy::process(requested, actual);
        }

        auto is_complete() const -> bool
        {
            return value() == 0 or maybe_eof_;
        }
    };
    
    class until : public size_policy
    {
    private:
        buffer_ref match_;
        //size_t match_idx_ = 0;
        bool matched_ = false;
    public:
        until(buffer_ref match, size_t buffer_size = 1024)
            : size_policy(buffer_size)
            , match_{match}
        {
        }
        
        auto is_complete() const -> bool
        {
            return matched_;
        }

        auto process(buffer_ref requested, buffer_ref actual) -> void
        {
            
        }
    };

    template<class... SizePolicies>
    class one_of final : public size_policy
    {
    private:
        using SizePolicyTuple = std::tuple<SizePolicies...>;
        SizePolicyTuple size_policies_;

        static constexpr const size_t tuple_size = std::tuple_size_v<SizePolicyTuple>;

        template<size_t Index>
        auto process_policies(buffer_ref requested, buffer_ref actual) -> void
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

        template<size_t Index>
        auto policy_value() const -> size_t
        {
            auto val = std::get<Index>(size_policies_).value();
            if constexpr (Index > 0)
            {
                return std::min(val, policy_value<Index-1>());
            }
            return val;
        }
    public:
        one_of(SizePolicies && ... size_policies)
            : size_policy(0) // TODO: fix this unused var
            , size_policies_{std::forward<SizePolicies>(size_policies)...}
        {
        }

        auto process(buffer_ref requested, buffer_ref actual) -> void
        {
            process_policies<tuple_size-1>(requested, actual);
        }

        auto is_complete() const -> bool
        {
            return is_policy_complete<tuple_size-1>();
        }

        auto value() const -> size_t
        {
            return policy_value<tuple_size-1>();
        }
    };
}
