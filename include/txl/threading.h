#pragma once

#include <functional>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstddef>
#include <thread>

namespace txl
{
    class threading_unit_test_base
    {
    private:
        static const constexpr uint8_t MAX_THREAD_SCALE = 10;
        std::mutex barrier_mutex_{};
        std::condition_variable barrier_{};
        std::vector<std::thread> threads_{};
        uint8_t thread_scale_ = 5;
        volatile bool ready_ = false;

        auto run_n_threads(size_t n) -> void
        {
            ready_ = false;
            auto start_mtx = std::mutex{};
            auto startup_cv = std::condition_variable{};
            threads_.clear();
            size_t num_running = 0;

            for (size_t i = 0; i < n; ++i)
            {
                threads_.emplace_back([this, i, &num_running, &start_mtx, &startup_cv]() {
                    {
                        std::unique_lock lock{start_mtx};
                        num_running++;
                    }
                    startup_cv.notify_all();
                    thread_body(i);
                });
            }

            // Wait for notification that all threads are active
            while (num_running != n)
            {
                {
                    std::unique_lock start_lk{start_mtx};
                    if (num_running != n)
                    {
                        startup_cv.wait(start_lk);
                    }
                }
            }

            pre_test();

            // Alert threads to do work
            {
                std::unique_lock lk{barrier_mutex_};
                ready_ = true;
            }
            barrier_.notify_all();

            // Wait
            join();
            post_test();
        }

        auto thread_body(size_t thread_num) -> void
        {
            // Wait to start...
            {
                std::unique_lock lock{barrier_mutex_};
                barrier_.wait(lock, [this]() { return ready_; });
            }
            // Run
            thread_main();
        }
    protected:
        virtual auto pre_test() -> void
        {
        }

        virtual auto thread_main() -> void = 0;
        
        virtual auto post_test() -> void
        {
        }
    public:
        auto run() -> void
        {
            // Scale up the number of threads
            size_t n = 1;
            for (uint8_t i = 0; i < thread_scale_; ++i)
            {
                run_n_threads(n);
                n <<= 1;
            }
        }

        auto join() -> void
        {
            for (auto & t : threads_)
            {
                t.join();
            }
        }

        auto set_thread_scale(uint8_t scale) -> uint8_t
        {
            thread_scale_ = std::min(std::max(scale, static_cast<uint8_t>(1)), MAX_THREAD_SCALE);
            return thread_scale_;
        }
    };
    
    class threading_unit_test : public threading_unit_test_base
    {
    private:
        std::function<void()> pre_test_;
        std::function<void()> test_body_;
        std::function<void()> post_test_;
        
        auto pre_test() -> void override
        {
            if (pre_test_)
            {
                pre_test_();
            }
        }

        auto post_test() -> void override
        {
            if (post_test_)
            {
                post_test_();
            }
        }
        
        auto thread_main() -> void override
        {
            test_body_();
        }
    public:
        threading_unit_test(std::function<void()> test_body)
            : test_body_(test_body)
        {
        }

        auto pre_test(std::function<void()> test) -> void
        {
            pre_test_ = test;
        }

        auto post_test(std::function<void()> test) -> void
        {
            post_test_ = test;
        }
    };
}
