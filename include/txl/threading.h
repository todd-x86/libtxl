#pragma once

#include <functional>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstddef>
#include <thread>

namespace txl
{
    class awaiter
    {
    private:
        struct awaiter_core final
        {
            std::condition_variable cond_;
            std::mutex mut_;
            // TODO: atomic_bool
            bool set_ = false;
            char padding_[128];

            auto reset() -> void
            {
                set_ = false;
            }

            auto wait() -> void
            {
                if (not set_)
                {
                    auto lock = std::unique_lock<std::mutex>{mut_};
                    cond_.wait(lock);
                }
            }

            auto notify_all() -> void
            {
                set_ = true;
                cond_.notify_all();
            }
        };

        std::shared_ptr<awaiter_core> core_{};
        char padding_[128];
    public:
        awaiter()
            : core_(std::make_shared<awaiter_core>())
        {
        }
        
        awaiter(awaiter const & a)
            : core_(a.core_)
        {
        }

        awaiter(awaiter && a)
            : core_(std::move(a.core_))
        {
        }
        
        auto operator=(awaiter && a) -> awaiter &
        {
            if (this != &a)
            {
                auto p = std::move(a.core_);
                core_ = std::move(p);
            }
            return *this;
        }
        
        auto operator=(awaiter const & a) -> awaiter &
        {
            if (this != &a)
            {
                auto p = a.core_;
                core_ = p;
            }
            return *this;
        }

        auto assign(awaiter & a) -> void
        {
            core_ = a.core_;
        }

        auto reset() -> void
        {
            auto p = core_;
            if (p)
            {
                p->reset();
            }
        }

        auto notify_all() -> void
        {
            auto p = core_;
            if (p)
            {
                p->notify_all();
            }
        }

        auto wait() -> void
        {
            auto p = core_;
            if (p)
            {
                p->wait();
            }
        }
    };
    
    struct thread_pool_work
    {
        virtual auto execute() -> void = 0;
        virtual auto next() -> bool = 0;
        virtual auto complete() -> void = 0;
    };

    class thread_pool_worker
    {
    private:
        struct pending_waiter final
        {
            std::condition_variable cond_;
            std::mutex mut_;
        };
        std::thread thread_;
        std::list<std::unique_ptr<thread_pool_work>> pending_;
        std::unique_ptr<pending_waiter> pending_waiter_;
        std::atomic_bool stopped_;

        auto thread_body() -> void
        {
            while (not stopped_.load(std::memory_order_relaxed))
            {
                auto work = get_work();
                if (work)
                {
                    do
                    {
                        work->execute();
                    }
                    while (not stopped_.load(std::memory_order_relaxed) and work->next());
                    work->complete();
                }
            }
        }

        auto get_work() -> std::unique_ptr<thread_pool_work>
        {
            while (not stopped_.load(std::memory_order_relaxed) and pending_.empty())
            {
                auto lock = std::unique_lock<std::mutex>{pending_waiter_->mut_};
                pending_waiter_->cond_.wait(lock, [this]() {
                    return not pending_.empty();
                });
            }
            if (stopped_.load(std::memory_order_relaxed) or pending_.empty())
            {
                return {};
            }

            auto w = std::move(pending_.front());
            pending_.pop_front();
            return {std::move(w)};
        }
    public:
        thread_pool_worker()
            : pending_waiter_(std::make_unique<pending_waiter>())
        {
        }

        thread_pool_worker(thread_pool_worker const &) = delete;

        thread_pool_worker(thread_pool_worker && w)
            : thread_(std::move(w.thread_))
            , pending_(std::move(w.pending_))
            , pending_waiter_(std::move(w.pending_waiter_))
            , stopped_()
        {
            auto old_value = stopped_.load();
            stopped_.store(w.stopped_.load());
            w.stopped_.store(old_value);
        }

        auto operator=(thread_pool_worker const &) -> thread_pool_worker & = delete;

        auto operator=(thread_pool_worker && w) -> thread_pool_worker &
        {
            if (this != &w)
            {
                thread_ = std::move(w.thread_);
                pending_ = std::move(w.pending_);
                pending_waiter_ = std::move(w.pending_waiter_);
                
                auto old_value = stopped_.load();
                stopped_.store(w.stopped_.load());
                w.stopped_.store(old_value);
            }
            return *this;
        }

        auto post(std::unique_ptr<thread_pool_work> && c) -> void
        {
            {
                auto lock = std::unique_lock<std::mutex>{pending_waiter_->mut_};
                pending_.emplace_back(std::move(c));
            }
            pending_waiter_->cond_.notify_one();
        }

        auto is_running() const -> bool
        {
            return not stopped_.load(std::memory_order_relaxed);
        }

        auto start() -> void
        {
            stopped_.store(false, std::memory_order_seq_cst);

            thread_ = std::thread([&]() {
                thread_body();
            });
        }

        auto stop() -> void
        {
            stopped_.store(true, std::memory_order_seq_cst);
            pending_waiter_->cond_.notify_all();
        }

        auto wait_for_shutdown() -> void
        {
            thread_.join();
        }
    };

    class thread_pool
    {
    private:
        std::vector<thread_pool_worker> workers_{};
        std::atomic<size_t> next_thread_index_ = 0;
    public:
        thread_pool(size_t num_threads)
        {
            workers_.resize(num_threads);
        }

        auto post_work(std::unique_ptr<thread_pool_work> && c) -> void
        {
            auto index = next_thread_index_.fetch_add(1);
            if (index == workers_.size())
            {
                next_thread_index_.store(0, std::memory_order_seq_cst);
                index = 0;
            }
            workers_[index].post(std::move(c));
        }

        auto start_workers() -> void
        {
            for (auto & w : workers_)
            {
                w.start();
            }
        }

        auto stop_workers() -> void
        {
            for (auto & w : workers_)
            {
                w.stop();
            }
            for (auto & w : workers_)
            {
                w.wait_for_shutdown();
            }
        }
    };

    class threading_unit_test_base
    {
    private:
        static const constexpr uint8_t MAX_THREAD_SCALE = 10;
        std::mutex barrier_mutex_{};
        std::condition_variable barrier_{};
        std::vector<std::thread> threads_{};
        uint8_t min_thread_scale_ = 0;
        uint8_t max_thread_scale_ = 5;
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
            size_t n = 1 << min_thread_scale_;
            for (uint8_t i = min_thread_scale_; i < max_thread_scale_; ++i)
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

        auto set_thread_scale(uint8_t min_scale, uint8_t max_scale) -> void
        {
            min_thread_scale_ = std::min(std::max(min_scale, static_cast<uint8_t>(1)), MAX_THREAD_SCALE);
            max_thread_scale_ = std::min(std::max(max_scale, static_cast<uint8_t>(1)), MAX_THREAD_SCALE);
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
