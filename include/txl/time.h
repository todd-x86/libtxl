#pragma once

#include <txl/types.h>

#include <time.h>

#include <chrono>
#include <iomanip>
#include <ostream>

namespace txl::time
{
    static constexpr const long NANOS_PER_SECOND = 1'000'000'000L;
    static constexpr const long MICROS_PER_SECOND = 1'000'000L;

    class time_point_formatter final
    {
        friend auto operator<<(std::ostream & os, time_point_formatter fmt) -> std::ostream &;
    private:
        std::chrono::system_clock::time_point const & tp_;
    public:
        time_point_formatter(std::chrono::system_clock::time_point const & tp)
            : tp_{tp}
        {
        }
    };

    inline auto format_time_point(std::chrono::system_clock::time_point const & tp) -> time_point_formatter
    {
        return {tp};
    }

    inline auto to_tm(std::chrono::system_clock::time_point tp) -> std::tm
    {
        std::tm res{};
        auto timer = std::chrono::system_clock::to_time_t(tp);
        // TODO: Check for null
        ::localtime_r(&timer, &res);
        return res;
    }

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
        return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::nanoseconds{total_nanos});
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
    
    template<class Clock, class Duration>
    inline auto to_time_point(std::chrono::seconds const & sec) -> std::chrono::time_point<Clock, Duration>
    {
        // TODO: too redundant?
        return std::chrono::time_point<Clock, Duration>{sec};
    }
    
    template<class TimePoint>
    inline auto to_time_point(std::chrono::seconds const & sec) -> TimePoint
    {
        // TODO: too redundant?
        return to_time_point<typename TimePoint::clock, typename TimePoint::duration>(sec);
    }


    template<class Rep, class Period>
    inline auto to_timeval(std::chrono::duration<Rep, Period> d) -> ::timeval
    {
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(d).count();
        auto secs = micros / MICROS_PER_SECOND;
        micros -= (secs * MICROS_PER_SECOND);
        return {secs, micros};
    }
    
    inline auto operator<<(std::ostream & os, time_point_formatter fmt) -> std::ostream &
    {
        auto time_tm = to_tm(fmt.tp_);
        os << std::put_time(&time_tm, "%Y%m%d-%H:%M:%S");
        return os;
    }
}
