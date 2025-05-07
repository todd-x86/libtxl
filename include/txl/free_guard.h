#pragma once

namespace txl
{
    template<class T>
    class free_guard final
    {
    public:
        T * owned_;
    public:
        free_guard(T * value)
            : owned_{value}
        {
        }

        ~free_guard()
        {
            if (owned_)
            {
                delete owned_;
            }
        }
    };
}
