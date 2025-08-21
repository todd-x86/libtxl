#pragma once

#include <cstddef>
#include <chrono>
#include <functional>
#include <string_view>

namespace txl
{
    class expect final
    {
    private:
        static thread_local std::function<void(expect const &, std::chrono::steady_clock::time_point const &)> on_at_most_;

        std::chrono::steady_clock::time_point start_;
        std::chrono::steady_clock::time_point limit_;
    public:
        static auto on_at_most(std::function<void(expect const &, std::chrono::steady_clock::time_point const &)> && callback) -> void
        {
            on_at_most_ = std::move(callback);
        }

        template<class Rep, class Period>
        expect(char const * filename, size_t line, std::chrono::duration<Rep, Period> const & duration)
            : start_(std::chrono::steady_clock::now())
            , limit_(start_ + duration)
        {
        }

        ~expect()
        {
            auto now = std::chrono::steady_clock::now();
            if (now > limit_ and on_at_most_)
            {
                on_at_most_(*this, now);
            }
        }

        template<class Duration>
        auto expected_time() const -> Duration
        {
            return std::chrono::duration_cast<Duration>(limit_ - start_);
        }

        template<class Duration>
        auto actual_time(std::chrono::steady_clock::time_point const & now) const -> Duration
        {
            return std::chrono::duration_cast<Duration>(now - start_);
        }
    };
}

#define _EXPECT_NAME(line) expect_line ## line
#define _EXPECT_AT_MOST(dur, line) auto _EXPECT_NAME(line) = ::txl::expect{__FILE__, __LINE__, dur}
#define EXPECT_AT_MOST(dur) _EXPECT_AT_MOST(dur, __LINE__)
