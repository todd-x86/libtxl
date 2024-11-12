#pragma once

#include <time.h>

#include <chrono>

namespace txl::time
{
    static constexpr const long NANOS_PER_SECOND = 1'000'000'000L;

    template<class Rep, class Period>
    inline auto to_timespec(std::chrono::duration<Rep, Period> d) -> ::timespec
    {
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
        auto secs = nanos / NANOS_PER_SECOND;
        nanos -= (secs * NANOS_PER_SECOND);
        return {secs, nanos};
    }


    template<class Clock, class Duration>
    inline auto to_timespec(std::chrono::time_point<Clock, Duration> tp) -> ::timespec
    {
        return to_timespec(tp.time_since_epoch());
    }

    template<class Rep, class Period>
    inline auto to_duration(::timespec const & ts) -> std::chrono::duration<Rep, Period>
    {
        auto total_nanos = (ts.tv_sec * NANOS_PER_SECOND) + ts.tv_nsec;
        return std::chrono::nanoseconds{total_nanos};
    }
    
    template<class Duration>
    inline auto to_duration(::timespec const & ts) -> std::chrono::duration<typename Duration::rep, typename Duration::period>
    {
        return to_duration<typename Duration::rep, typename Duration::period>(ts);
    }
    
    template<class Clock, class Duration>
    inline auto to_time_point(::timespec const & ts) -> std::chrono::time_point<Clock, Duration>
    {
        using time_point = std::chrono::time_point<Clock, Duration>;
        return time_point{to_duration<typename time_point::rep, typename time_point::period>(ts)};
    }

    template<class TimePoint>
    inline auto to_time_point(::timespec const & ts) -> TimePoint
    {
        return to_time_point<typename TimePoint::clock, typename TimePoint::duration>(ts);
    }
}
