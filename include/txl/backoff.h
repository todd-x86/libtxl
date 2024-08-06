#pragma once

namespace txl
{
    /**
     * Requirements for Sleep:
     *     Sleep();
     *     void operator()(uint64_t nanoseconds);
     */
    template<class Sleep>
    class scaled_backoff
    {
    private:
        uint64_t time_ = 15;
        static constexpr uint64_t mask_ = 0x7FFFFFFFFFFFFFF;
    public:
        void wait()
        {
            Sleep()(time_);
            time_ = ((time_ << 1) | 1) & mask_;
        }

        void reset()
        {
            time_ = 15;
        }
    };
}
