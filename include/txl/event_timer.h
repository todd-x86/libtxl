#pragma once

#include <txl/file_base.h>
#include <txl/result.h>
#include <txl/system_error.h>
#include <txl/handle_error.h>
#include <txl/time.h>

#include <sys/timerfd.h>
#include <time.h>
#include <chrono>

namespace txl
{
    struct event_timer : file_base
    {
        enum timer_type
        {
            system_clock = CLOCK_REALTIME,
            steady_clock = CLOCK_MONOTONIC,
        };

        event_timer() = default;

        event_timer(std::chrono::system_clock::time_point tp)
        {
            open(system_clock).or_throw();
            set_time(tp).or_throw();
        }

        event_timer(std::chrono::steady_clock::time_point tp)
        {
            open(steady_clock).or_throw();
            set_time(tp).or_throw();
        }

        auto open(timer_type tt) -> result<void>
        {
            if (is_open())
            {
                return get_system_error(EBUSY);
            }
            fd_ = ::timerfd_create(static_cast<int>(tt), 0);
            return handle_system_error(fd_);
        }

        template<class Clock, class Duration>
        auto set_time(std::chrono::time_point<Clock, Duration> tp, std::chrono::nanoseconds interval = std::chrono::nanoseconds{0}) -> result<void>
        {
            auto timer_val = ::itimerspec{time::to_timespec(interval), time::to_timespec(tp)};
            auto res = ::timerfd_settime(fd_, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &timer_val, nullptr);
            return handle_system_error(res);
        }
    };
}
