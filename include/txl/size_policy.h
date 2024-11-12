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
}
