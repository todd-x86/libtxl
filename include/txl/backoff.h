#pragma once

namespace txl
{
    /**
     * Simple backoff mechanism that scales exponentially by powers of 2.
     *
     * Requirements for Sleep:
     *     Sleep();
     *     void operator()(uint64_t nanoseconds);
     */
    template<class Sleep, uint64_t UpperBoundMask = 0x7FFF>
    class scaled_backoff
    {
    private:
        uint64_t time_ = 15;
    public:
        /**
         * Invokes the sleeper with the desired backoff time. 
         * Increases the duration for the next call to wait().
         * Time is bounded to avoid scaling 
         */
        auto wait() -> void
        {
            Sleep()(time_);
            time_ = ((time_ << 1) | 1) & UpperBoundMask;
        }

        auto reset() -> void
        {
            time_ = 15;
        }
    };
}
