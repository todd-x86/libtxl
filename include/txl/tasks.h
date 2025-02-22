#pragma once

#include <txl/on_exit.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace txl
{
    template<class ReturnType>
    class task_context;

    struct future_base
    {
        virtual auto wait() -> void = 0;
    };

    template<class ReturnType>
    class future : public future_base
    {
    private:
        std::future<ReturnType> fut_;
    public:
        future(std::future<ReturnType> fut)
            : fut_(std::move(fut))
        {
        }

        auto wait() -> void override
        {
            fut_.wait();
        }
    };

    namespace detail
    {
        class task_context_base
        {
        private:
            bool success_ = true;
       
            auto flag_error() -> void
            {
                success_ = false;
            }
        public:
            auto is_success() const -> bool
            {
                return success_;
            }
        };
    }  

    template<class ReturnType>
    class task_context : public detail::task_context_base
    {
    private:
        std::optional<ReturnType> ret_;
    public:
        auto set_result(ReturnType && value) -> void
        {
            ret_ = std::make_optional(std::move(value));
        }
        
        auto result() const -> ReturnType const &
        {
            return *ret_;
        }

        auto result() -> ReturnType &
        {
            return *ret_;
        }

        auto release_result() -> ReturnType &&
        {
            return std::move(*ret_);
        }
    };
    
    template<>
    struct task_context<void> : detail::task_context_base
    {
    };


    namespace detail
    {
        template<class ReturnType>
        class task_work
        {
        private:
            std::packaged_task<ReturnType(task_context<ReturnType> &)> task_;
            std::unique_ptr<task_work<ReturnType>> next_;
        public:
            task_work(std::function<ReturnType()> && func)
                : task_([func=std::move(func)](task_context<ReturnType> &) {
                    return func();
                })
            {
            }
            
            task_work(std::function<ReturnType(task_context<ReturnType> &)> && func)
                : task_(std::move(func))
            {
            }

            task_work(std::promise<void> && p)
                : task_([p=std::move(p)](task_context<ReturnType> & ctx) mutable {
                    auto _ = txl::on_exit([&]() {
                        p.set_value();
                    });
                    if constexpr (not std::is_void_v<ReturnType>)
                    {
                        return ctx.release_result();
                    }
                })
            {
            }

            auto get_completion_future() -> std::unique_ptr<future_base>
            {
                return std::make_unique<future<ReturnType>>(std::future<ReturnType>{task_.get_future()});
            }

            auto wait() -> void
            {
                task_.get_future().wait();
            }

            auto chain(std::unique_ptr<task_work<ReturnType>> && next) -> void
            {
                next_ = std::move(next);
            }

            auto execute(task_context<ReturnType> & ctx) -> ReturnType
            {
                task_.reset();
                task_(ctx);
                return task_.get_future().get();
            }
            
            auto next() -> task_work<ReturnType> * 
            {
                return next_.get();
            }

            auto tail() -> task_work *
            {
                auto next = this;
                auto prev = next;
                while (next)
                {
                    prev = next;
                    next = next->next_.get();
                }
                return prev;
            }

            auto count() const -> int
            {
                auto count = 0;
                auto next = this;
                while (next)
                {
                    ++count;
                    next = next->next_.get();
                }
                return count;
            }
        };
    }
    
    template<class ReturnType>
    class task;

    struct closure
    {
        virtual auto execute() -> void = 0;
        virtual auto next() -> bool = 0;
        virtual auto copy() -> std::unique_ptr<closure> = 0;
        virtual auto get_completion_task() -> task<void> = 0;
    };

    class task_runner
    {
    private:
        static std::unique_ptr<task_runner> global_;
    public:
        static auto global() -> task_runner &
        {
            return *global_;
        }

        static auto set_global(std::unique_ptr<task_runner> && r) -> void
        {
            global_ = std::move(r);
        }

        virtual auto run(closure & c) -> task<void> = 0;
        virtual auto delay(int64_t timeout_nanos) -> task<void> = 0;
    };

    template<class ReturnType>
    class task
    {
    private:
        std::unique_ptr<detail::task_work<ReturnType>> work_;
        detail::task_work<ReturnType> * tail_ = nullptr;
        std::unique_ptr<future_base> future_;

        task(std::unique_ptr<detail::task_work<ReturnType>> && w)
            : work_(std::move(w))
            , tail_(work_.get())
            , future_(work_->get_completion_future())
        {
        }

        auto add_one_work(std::unique_ptr<detail::task_work<ReturnType>> && next) -> void
        {
            auto new_tail = next.get();
            if (work_)
            {
                tail_->chain(std::move(next));
            }
            else
            {
                work_ = std::move(next);
            }
            tail_ = new_tail;
            future_ = tail_->get_completion_future();
            assert(tail_ == work_->tail());
        }

        auto add_multi_work(std::unique_ptr<detail::task_work<ReturnType>> && next, detail::task_work<ReturnType> * new_tail) -> void
        {
            if (work_)
            {
                assert(tail_ == work_->tail());
                tail_->chain(std::move(next));
            }
            else
            {
                work_ = std::move(next);
            }
            tail_ = new_tail;
            future_ = tail_->get_completion_future();
        }
    public:
        task() = default;
        
        task(std::unique_ptr<future_base> && fut)
            : future_(std::move(fut))
        {
        }
        
        task(std::function<ReturnType()> && f)
            : task(std::make_unique<detail::task_work<ReturnType>>(std::move(f)))
        {
        }

        task(task const &) = delete;

        task(task && t)
            : work_(std::move(t.work_))
        {
            tail_ = t.tail_;
            future_ = std::move(t.future_);
            t.tail_ = nullptr;
        }

        auto operator=(task const &) -> task & = delete;

        auto operator=(task && t) -> task &
        {
            if (this != &t)
            {
                work_ = std::move(t.work_);
                future_ = std::move(t.future_);
                tail_ = t.tail_;
                t.tail_ = nullptr;
            }
            return *this;
        }

        auto empty() const -> bool { return not static_cast<bool>(future_); }

        auto wait() -> void
        {
            if (future_)
            {
                assert(work_);
                assert(tail_);
                future_->wait();
            }
        }
        
        auto then(task<ReturnType> && t) -> task<ReturnType> &&
        {
            if (this == &t or t.empty())
            {
                return std::move(*this);
            }

            auto new_tail = t.work_->tail();
            add_multi_work(std::move(t.work_), new_tail);
            t.tail_ = nullptr;

            return std::move(*this);
        }

        auto then(std::function<ReturnType()> && f) -> task<ReturnType> &&
        {
            auto next = std::make_unique<detail::task_work<ReturnType>>(std::move(f));
            add_one_work(std::move(next));
            return std::move(*this);
        }

        auto then(std::function<ReturnType(task_context<ReturnType> &)> && f) -> task<ReturnType> &&
        {
            auto next = std::make_unique<detail::task_work<ReturnType>>(std::move(f));
            add_one_work(std::move(next));
            return std::move(*this);
        }

        auto operator()() -> ReturnType
        {
            return (*this)(task_runner::global());
        }

        auto operator()(task_runner & runner) -> ReturnType;
    };
    
    template<class ReturnType>
    class task_work_closure final : public closure
    {
    private:
        detail::task_work<ReturnType> * work_;
        task_context<ReturnType> & ctx_;
    public:
        task_work_closure(detail::task_work<ReturnType> & work, task_context<ReturnType> & ctx)
            : work_(&work)
            , ctx_(ctx)
        {
        }

        auto execute() -> void override
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                work_->execute(ctx_);
            }
            else
            {
                auto res = work_->execute(ctx_);
                ctx_.set_result(std::move(res));
            }
        }

        auto next() -> bool override
        {
            work_ = work_->next();
            return work_ != nullptr;
        }

        auto copy() -> std::unique_ptr<closure> override
        {
            return std::make_unique<task_work_closure>(*work_, ctx_);
        }
        
        auto get_completion_task() -> task<void> override
        {
            if (work_)
            {
                std::promise<void> completion{};
                auto fut = completion.get_future();
                if constexpr (std::is_void_v<ReturnType>)
                {
                    work_->chain(std::make_unique<detail::task_work<void>>(std::move(completion)));
                }
                else
                {
                    work_->chain(std::make_unique<detail::task_work<ReturnType>>(std::move(completion)));
                }
                return {std::make_unique<future<void>>(std::move(fut))};
            }
            return {};
        }
    };

    struct inline_task_runner : task_runner
    {
        auto run(closure & c) -> task<void> override
        {
            do
            {
                c.execute();
            }
            while (c.next());

            return {};
        }

        auto delay(int64_t timeout_nanos) -> task<void> override
        {
            return {[timeout_nanos]() {
                std::this_thread::sleep_for(std::chrono::nanoseconds{timeout_nanos});
            }};
        }
    };

    std::unique_ptr<task_runner> task_runner::global_{std::make_unique<inline_task_runner>()};

    class thread_pool_worker
    {
    private:
        struct pending_waiter final
        {
            std::condition_variable cond_;
            std::mutex mut_;
        };
        std::thread thread_;
        std::list<std::unique_ptr<closure>> pending_;
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
                }
            }
        }

        auto get_work() -> std::unique_ptr<closure>
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
        thread_pool_worker() = default;

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

        auto post(std::unique_ptr<closure> && c) -> task<void>
        {
            task<void> completion_task = c->get_completion_task();
            pending_.emplace_back(std::move(c));
            pending_waiter_->cond_.notify_one();
            return completion_task;
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

        auto post_work(std::unique_ptr<closure> && c) -> task<void>
        {
            auto index = next_thread_index_.fetch_add(1);
            if (index == workers_.size())
            {
                next_thread_index_.store(0, std::memory_order_seq_cst);
            }
            return workers_[index].post(std::move(c));
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

    struct thread_pool_task_runner : task_runner
    {
    private:
        thread_pool pool_;
    public:
        thread_pool_task_runner(size_t num_threads)
            : pool_{num_threads}
        {
            pool_.start_workers();
        }

        ~thread_pool_task_runner()
        {
            pool_.stop_workers();
        }

        auto run(closure & c) -> task<void> override
        {
            return pool_.post_work(c.copy());
        }

        auto delay(int64_t timeout_nanos) -> task<void> override
        {
            return {[timeout_nanos]() {
                std::this_thread::sleep_for(std::chrono::nanoseconds{timeout_nanos});
            }};
        }
    };

    template<class ReturnType>
    inline auto task<ReturnType>::operator()(task_runner & runner) -> ReturnType
    {
        auto ctx = task_context<ReturnType>{};
        auto closure = task_work_closure<ReturnType>{*work_, ctx};
        auto fut = runner.run(closure);
        fut.wait();
        if constexpr (not std::is_same_v<ReturnType, void>)
        {
            return ctx.release_result();
        }
    }

    template<class Rep, class Period>
    inline auto delay(std::chrono::duration<Rep, Period> const & sleep) -> task<void>
    {
        return task_runner::global().delay(std::chrono::duration_cast<std::chrono::nanoseconds>(sleep).count());
    }

    template<class ReturnType>
    inline auto make_task(std::function<ReturnType()> && f) -> task<ReturnType>
    {
        return {std::move(f)};
    }
    
    inline auto make_task(std::function<void()> && f) -> task<void>
    {
        return {std::move(f)};
    }
}
