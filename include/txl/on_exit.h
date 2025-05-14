#pragma once

#include <functional>

namespace txl
{
    class on_exit final
    {
    private:
        std::function<void()> exec_;
    public:
        on_exit(std::function<void()> && exec)
            : exec_(std::move(exec))
        {
        }

        ~on_exit()
        {
            if (exec_)
            {
                exec_();
            }
        }
    };
}
