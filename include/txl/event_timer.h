#pragma once

#include <txl/file_base.h>
#include <txl/on_error.h>
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

        event_timer(std::chrono::system_clock::time_point tp, on_error::callback<system_error> on_err = on_error::throw_on_error{})
        {
            open(system_clock, on_err);
            set_time(tp);
        }

        event_timer(std::chrono::steady_clock::time_point tp, on_error::callback<system_error> on_err = on_error::throw_on_error{})
        {
            open(steady_clock, on_err);
            set_time(tp);
        }

        auto open(timer_type tt, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            if (is_open())
            {
                on_err(EBUSY);
            }
            fd_ = ::timerfd_create(static_cast<int>(tt), 0);
            handle_system_error(fd_, on_err);
        }

        template<class Clock, class Duration>
        auto set_time(std::chrono::time_point<Clock, Duration> tp, std::chrono::nanoseconds interval = std::chrono::nanoseconds{0}, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto timer_val = ::itimerspec{time::to_timespec(interval), time::to_timespec(tp)};
            auto res = ::timerfd_settime(fd_, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &timer_val, nullptr);
            handle_system_error(res, on_err);
        }
    };
}