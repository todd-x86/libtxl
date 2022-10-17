#pragma once

// TODO: do we need this class?
#include "sleep.h"

namespace llio
{
    class scaled_backoff
    {
    private:
        uint64_t M_time_ = 15;
        static constexpr uint64_t M_mask_ = 0x7FFFFFF;
    public:
        void wait()
        {
            ::llio::nanosleep(M_time_);
            M_time_ = ((M_time_ << 1) | 1) & M_mask_;
        }

        void reset()
        {
            M_time_ = 1;
        }
    };
}
