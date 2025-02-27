#pragma once

#include <txl/threading.h>
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
#include <type_traits>

namespace txl
{
    template<class ReturnType>
    class future
    {
    private:
        awaiter awaiter_;
    public:
        future(awaiter const & a)
            : awaiter_(a)
        {
        }

        auto wait() -> void
        {
            awaiter_.wait();
        }
    };

    template<class ReturnType>
    class promise
    {
    private:
        std::optional<ReturnType> value_{};
        awaiter awaiter_{};

        auto notify_awaiters() -> void
        {
            awaiter_.notify_all();
        }
    public:
        promise() = default;
        promise(promise && p)
            : value_(std::move(p.value_))
            , awaiter_(std::move(p.awaiter_))
        {
        }
        
        auto operator=(promise && p) -> promise &
        {
            if (this != &p)
            {
                value_ = std::move(p.value_);
                awaiter_ = std::move(p.awaiter_);
            }
            return *this;
        }

        auto get_future() const -> future<ReturnType>
        {
            return {awaiter_};
        }

        auto move_from(promise<ReturnType> && p) -> void
        {
            value_ = std::move(p.value_);
            p.awaiter_.assign(awaiter_);
        }

        auto release_value() -> ReturnType &&
        {
            return std::move(*value_);
        }

        auto get_value() const -> ReturnType const &
        {
            return *value_;
        }

        auto has_value() const -> bool { return value_.has_value(); }
        
        auto set_value(ReturnType && val) -> void
        {
            value_ = std::move(val);
            notify_awaiters();
        }
    };
    
    template<>
    class promise<void>
    {
    private:
        awaiter awaiter_{};
        // TODO: atomic_bool
        bool set_ = false;

        auto notify_awaiters() -> void
        {
            awaiter_.notify_all();
        }
    public:
        promise() = default;
        promise(promise && p)
            : awaiter_(std::move(p.awaiter_))
        {
            std::swap(set_, p.set_);
        }

        auto operator=(promise && p) -> promise &
        {
            if (this != &p)
            {
                awaiter_ = std::move(p.awaiter_);
                std::swap(set_, p.set_);
            }
            return *this;
        }
        
        auto move_from(promise<void> && p) -> void
        {
            set_ = p.set_;
            p.awaiter_.assign(awaiter_);
        }

        auto get_future() const -> future<void>
        {
            return {awaiter_};
        }

        auto has_value() const -> bool { return set_; }
        
        auto set_value() -> void
        {
            set_ = true;
            notify_awaiters();
        }
    };

    template<class ReturnType>
    class task_context
    {
    private:
        promise<ReturnType> * prom_ = nullptr;
        bool success_ = false;
    public:
        task_context() = default;

        task_context(promise<ReturnType> & prom)
            : prom_(&prom)
            , success_(true)
        {
        }

        auto is_success() const -> bool { return success_; }

        auto result() const -> ReturnType const &
        {
            return prom_->get_value();
        }

        auto set_result(ReturnType && v) -> void
        {
            prom_->set_value(std::move(v));
        }
    };

    template<>
    class task_context<void>
    {
    private:
        promise<void> * prom_ = nullptr;
        bool success_ = false;
    public:
        task_context() = default;

        task_context(promise<void> & prom)
            : prom_(&prom)
            , success_(true)
        {
        }

        auto is_success() const -> bool { return success_; }
    };

    template<class ReturnType>
    struct task_function
    {
        virtual auto execute(task_context<ReturnType> & ctx) -> void = 0;
    };

    template<class ReturnType, class Func>
    class task_lambda_function : public task_function<ReturnType>
    {
    private:
        Func func_;
    public:
        task_lambda_function(Func && f)
            : func_(std::move(f))
        {
        }

        auto execute(task_context<ReturnType> & ctx) -> void override
        {
            if constexpr (std::is_void_v<ReturnType>)
            {
                func_(ctx);
            }
            else
            {
                ctx.set_result(func_(ctx));
            }
        }
    };

    template<class ReturnType>
    class task_chain
    {
    private:
        // TODO: sharing promise when moving task but keeping closures the same?
        promise<ReturnType> prom_;
        std::unique_ptr<task_function<ReturnType>> work_{nullptr};
        std::unique_ptr<task_chain> next_{nullptr};
    public:
        task_chain() = default;

        task_chain(promise<ReturnType> && p)
            : prom_(std::move(p))
        {
        }

        task_chain(task_chain && tc)
            : prom_(std::move(tc.prom_))
            , work_(std::move(tc.work_))
            , next_(std::move(tc.next_))
        {
        }

        template<class Func>
        task_chain(Func && f)
        {
            if constexpr (std::is_invocable_v<Func, task_context<ReturnType> &>)
            {
                work_ = std::make_unique<task_lambda_function<ReturnType, decltype(f)>>(std::move(f));
            }
            else //if constexpr (std::is_invocable_v<Func>)
            {
                if constexpr (std::is_void_v<ReturnType>)
                {
                    auto wrapper = [func=std::move(f)](task_context<ReturnType> &) mutable {
                        func();
                    };
                    work_ = std::make_unique<task_lambda_function<ReturnType, decltype(wrapper)>>(std::move(wrapper));
                }
                else
                {
                    auto wrapper = [func=std::move(f)](task_context<ReturnType> &) mutable {
                        return func();
                    };
                    work_ = std::make_unique<task_lambda_function<ReturnType, decltype(wrapper)>>(std::move(wrapper));
                }
            }
        }

        auto operator=(task_chain && tc) -> task_chain &
        {
            if (this != &tc)
            {
                prom_ = std::move(tc.prom_);
                work_ = std::move(tc.work_);
                next_ = std::move(tc.next_);
            }
            return *this;
        }

        auto reset_promises() -> void
        {
            auto * p = this;
            while (p)
            {
                p->prom_ = promise<ReturnType>{};
                p = p->next();
            }
        }

        auto execute_top(task_context<ReturnType> & ctx) -> void
        {
            if (not work_)
            {
                if constexpr (std::is_void_v<ReturnType>)
                {
                    // By nature of a non-returning task, we must acknowledge
                    // that it can be empty and thus if we execute it we must
                    // enforce the promise.
                    prom_.set_value();
                }
                return;
            }

            if constexpr (std::is_void_v<ReturnType>)
            {
                work_->execute(ctx);
                // TODO: move to task_context
                prom_.set_value();
            }
            else
            {
                work_->execute(ctx);
            }
        }

        auto get_promise() -> promise<ReturnType> &
        {
            return prom_;
        }

        auto wait() -> void
        {
            prom_.get_future().wait();
        }

        auto tail() -> task_chain *
        {
            auto prev = this;
            auto next = this;
            while (next)
            {
                prev = next;
                next = next->next();
            }
            return prev;
        }

        auto chain(std::unique_ptr<task_chain> && tc) -> void
        {
            tail()->next_ = std::move(tc);
        }

        auto empty() const -> bool
        {
            return not static_cast<bool>(work_);
        }

        auto next() -> task_chain *
        {
            return next_.get();
        }
    };

    class task_runner;

    template<class ReturnType>
    class task_core
    {
    protected:
        task_chain<ReturnType> chain_;

        auto assign(task_chain<ReturnType> && tc) -> void
        {
            chain_ = std::move(tc);
        }
        
        auto append(task_chain<ReturnType> && tc) -> void
        {
            chain_.chain(std::make_unique<task_chain<ReturnType>>(std::move(tc)));
        }

        auto reset_all_promises() -> void
        {
            chain_.reset_promises();
        }
    public:
        task_core() = default;

        task_core(promise<ReturnType> && p)
            : chain_(std::move(p))
        {
        }
        
        template<class Func>
        task_core(Func && f)
            : chain_(std::move(f))
        {
        }

        task_core(task_core const &) = delete;

        task_core(task_core && t)
            : chain_(std::move(t.chain_))
        {
        }

        auto operator=(task_core const &) -> task_core & = delete;

        auto operator=(task_core && t) -> task_core &
        {
            if (this != &t)
            {
                chain_ = std::move(t.chain_);
            }
            return *this;
        }
        
        auto get_promise() -> promise<ReturnType> &
        {
            return chain_.get_promise();
        }

        auto empty() const -> bool
        {
            return chain_.empty();
        }

        auto wait() -> void
        {
            chain_.wait();
        }
        
        auto then(task_core && t) -> void
        {
            if (chain_.empty())
            {
                assign(std::move(t.chain_));
            }
            else
            {
                append(std::move(t.chain_));
            }
        }

        template<class Func>
        auto then(Func && f) -> void
        {
            if (chain_.empty())
            {
                assign(task_chain<ReturnType>{std::move(f)});
            }
            else
            {
                append(task_chain<ReturnType>{std::move(f)});
            }
        }
    };

    template<class ReturnType>
    struct task : task_core<ReturnType>
    {
        using task_core<ReturnType>::task_core;
        
        auto then(task && t) -> task &&
        {
            task_core<ReturnType>::then(std::move(t));
            return std::move(*this);
        }

        template<class Func>
        auto then(Func && f) -> task &&
        {
            task_core<ReturnType>::then(std::move(f));
            return std::move(*this);
        }

        auto operator()() -> ReturnType &&;

        auto operator()(task_runner & runner) -> ReturnType &&;
    };
    
    template<>
    struct task<void> : task_core<void>
    {
        using task_core<void>::task_core;
        
        auto then(task && t) -> task &&
        {
            task_core<void>::then(std::move(t));
            return std::move(*this);
        }

        template<class Func>
        auto then(Func && f) -> task &&
        {
            task_core<void>::then(std::move(f));
            return std::move(*this);
        }
        
        auto operator()() -> void;

        auto operator()(task_runner & runner) -> void;
    };

    struct closure
    {
        virtual auto move() const -> std::unique_ptr<closure> = 0;
        virtual auto execute() -> void = 0;
        virtual auto next() -> bool = 0;
    };

    template<class ReturnType>
    class task_closure : public closure
    {
    private:
        task_chain<ReturnType> * chain_;
        task_context<ReturnType> ctx_;
    public:
        task_closure(task_chain<ReturnType> & chain)
            : chain_(&chain)
        {
        }

        task_closure(task_closure const &) = default;
        task_closure(task_closure &&) = default;
        
        auto move() const -> std::unique_ptr<closure> override
        {
            return std::make_unique<task_closure<ReturnType>>(std::move(*this));
        }

        auto execute() -> void override
        {
            ctx_ = task_context<ReturnType>{chain_->get_promise()};
            chain_->execute_top(ctx_);
        }

        auto next() -> bool override
        {
            if (chain_)
            {
                auto & prev_promise = chain_->get_promise();
                chain_ = chain_->next();
                
                // Move promise forward
                if (chain_)
                {
                    chain_->get_promise().move_from(std::move(prev_promise));
                    return true;
                }
            }

            return false;
        }
    };

    class task_runner
    {
    private:
        static std::unique_ptr<task_runner> global_;
    public:
        virtual auto run(closure && c) -> void = 0;
        virtual auto delay(uint64_t nanos) -> task<void> = 0;
        
        static auto set_global(std::unique_ptr<task_runner> && runner) -> void
        {
            global_ = std::move(runner);
        }

        static auto global() -> task_runner &
        {
            return *global_;
        }
    };

    struct inline_task_runner : task_runner
    {
        auto run(closure && c) -> void override
        {
            do
            {
                c.execute();
            }
            while (c.next());
        }

        auto delay(uint64_t nanos) -> task<void> override
        {
            return {[nanos]() {
                std::this_thread::sleep_for(std::chrono::nanoseconds(nanos));
            }};
        }
    };

    std::unique_ptr<task_runner> task_runner::global_(std::make_unique<inline_task_runner>());

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

        auto post(std::unique_ptr<closure> && c) -> void
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

        auto post_work(std::unique_ptr<closure> && c) -> void
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

        auto run(closure && c) -> void override
        {
            pool_.post_work(c.move());
        }

        auto delay(uint64_t timeout_nanos) -> task<void> override
        {
            return {[timeout_nanos]() {
                std::this_thread::sleep_for(std::chrono::nanoseconds{timeout_nanos});
            }};
        }
    };
    
    template<class ReturnType>
    auto task<ReturnType>::operator()() -> ReturnType &&
    {
        return (*this)(task_runner::global());
    }
    
    auto task<void>::operator()() -> void
    {
        (*this)(task_runner::global());
    }

    template<class ReturnType>
    auto task<ReturnType>::operator()(task_runner & runner) -> ReturnType &&
    {
        this->reset_all_promises();
        auto * tail = this->chain_.tail();
        runner.run(task_closure<ReturnType>{this->chain_});
        tail->get_promise().get_future().wait();
        return tail->get_promise().release_value();
    }
    
    auto task<void>::operator()(task_runner & runner) -> void
    {
        this->reset_all_promises();
        auto * tail = this->chain_.tail();
        runner.run(task_closure<void>{this->chain_});
        tail->get_promise().get_future().wait();
    }
    
    template<class Rep, class Period>
    inline auto delay(std::chrono::duration<Rep, Period> const & sleep) -> task<void>
    {
        return task_runner::global().delay(std::chrono::duration_cast<std::chrono::nanoseconds>(sleep).count());
    }

    template<class ReturnType, class Func>
    inline auto make_task(Func && f) -> task<ReturnType>
    {
        return {std::move(f)};
    }
}
