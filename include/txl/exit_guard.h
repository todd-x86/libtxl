#pragma once

#include <functional>

#define _TXL_DEFER_NAME(line) _exit_guard_line_ ## line
#define _TXL_DEFER(line) ::txl::exit_guard _TXL_DEFER_NAME(line) = [&]()
#define DEFER _TXL_DEFER(__LINE__)

namespace txl
{
    class exit_guard final
    {
    private:
        std::function<void()> on_exit_;
    public:
        exit_guard() = default;

        template<class Func>
        exit_guard(Func && func)
            : on_exit_{std::move(func)}
        {
        }

        ~exit_guard()
        {
            if (on_exit_)
            {
                on_exit_();
            }
        }
    };
}
