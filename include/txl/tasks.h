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
#include <type_traits>

namespace txl
{
    class awaiter
    {
    private:
        struct awaiter_core final
        {
            std::condition_variable cond_;
            std::mutex mut_;

            auto wait() -> void
            {
                auto lock = std::unique_lock<std::mutex>{mut_};
                cond_.wait(lock);
            }

            auto notify_all() -> void
            {
                cond_.notify_all();
            }
        };

        std::shared_ptr<awaiter_core> core_{};
    public:
        awaiter()
            : core_(std::make_shared<awaiter_core>())
        {
        }

        auto notify_all() -> void
        {
            core_->notify_all();
        }

        auto wait() -> void
        {
            core_->wait();
        }
    };

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
        std::optional<ReturnType> value_;
        bool success_ = true;
    public:
        auto is_success() const -> bool { return success_; }

        auto result() const -> ReturnType const &
        {
            return *value_;
        }

        auto release_result() -> ReturnType &&
        {
            return std::move(*value_);
        }

        auto set_result(ReturnType && v) -> void
        {
            value_ = std::move(v);
        }
    };

    template<>
    class task_context<void>
    {
    private:
        bool success_ = true;
    public:
        task_context() = default;

        auto is_success() const -> bool { return success_; }
    };

    template<class ReturnType>
    struct task_function
    {
        virtual auto set_context(task_context<ReturnType> & ctx) -> void = 0;
        virtual auto execute() -> void = 0;
    };

    template<class ReturnType, class Func>
    class task_lambda_function : public task_function<ReturnType>
    {
    private:
        Func func_;
        task_context<ReturnType> * ctx_;
    public:
        task_lambda_function(Func && f)
            : func_(std::move(f))
        {
        }

        auto set_context(task_context<ReturnType> & ctx) -> void
        {
            ctx_ = &ctx;
        }

        auto execute() -> void override
        {
            if constexpr (std::is_void_v<ReturnType>)
            {
                func_(*ctx_);
            }
            else
            {
                ctx_->set_result(func_(*ctx_));
            }
        }
    };

    template<class ReturnType>
    class task_chain
    {
    private:
        promise<ReturnType> prom_;
        std::unique_ptr<task_function<ReturnType>> work_;
        std::unique_ptr<task_chain> next_{};
    public:
        task_chain() = default;

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
            else if constexpr (std::is_invocable_v<Func>)
            {
                if constexpr (std::is_void_v<ReturnType>)
                {
                    auto wrapper = [func=std::move(f)](task_context<ReturnType> &) {
                        func();
                    };
                    work_ = std::make_unique<task_lambda_function<ReturnType, decltype(wrapper)>>(std::move(wrapper));
                }
                else
                {
                    auto wrapper = [func=std::move(f)](task_context<ReturnType> &) {
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

        auto execute_top(task_context<ReturnType> & ctx) -> void
        {
            if (not work_)
            {
                return;
            }

            if constexpr (std::is_void_v<ReturnType>)
            {
                work_->set_context(ctx);
                work_->execute();
                prom_.set_value();
            }
            else
            {
                work_->set_context(ctx);
                work_->execute();
                // TODO: fixme
                prom_.set_value(ctx.release_result());
            }
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
            return static_cast<bool>(work_);
        }

        auto next() -> task_chain *
        {
            return next_.get();
        }
    };

    class task_runner;

    template<class ReturnType>
    class task
    {
    private:
        task_chain<ReturnType> chain_;

        auto assign(task_chain<ReturnType> && tc) -> void
        {
            chain_ = std::move(tc);
        }
        
        auto append(task_chain<ReturnType> && tc) -> void
        {
            chain_.chain(std::make_unique<task_chain<ReturnType>>(std::move(tc)));
        }
    public:
        task() = default;
        
        template<class Func>
        task(Func && f)
            : chain_(std::move(f))
        {
        }

        task(task const &) = delete;

        task(task && t)
            : chain_(std::move(t.chain_))
        {
        }

        auto operator=(task const &) -> task & = delete;

        auto operator=(task && t) -> task &
        {
            if (this != &t)
            {
                chain_ = std::move(t.chain_);
            }
            return *this;
        }

        auto empty() const -> bool
        {
            return chain_.empty();
        }

        auto wait() -> void
        {
            chain_.wait();
        }
        
        auto then(task<ReturnType> && t) -> task<ReturnType> &&
        {
            if (chain_.empty())
            {
                assign(std::move(t.chain_));
            }
            else
            {
                append(std::move(t.chain_));
            }

            return std::move(*this);
        }

        template<class Func>
        auto then(Func && f) -> task<ReturnType> &&
        {
            if (chain_.empty())
            {
                assign(task_chain<ReturnType>{std::move(f)});
            }
            else
            {
                append(task_chain<ReturnType>{std::move(f)});
            }
            return std::move(*this);
        }

        auto operator()() -> ReturnType;
        auto operator()(task_runner & runner) -> ReturnType;
    };

    struct closure
    {
        virtual auto execute() -> void = 0;
    };

    template<class ReturnType>
    class task_closure : public closure
    {
    private:
        task_chain<ReturnType> * chain_;
        task_context<ReturnType> * ctx_;
    public:
        task_closure(task_chain<ReturnType> & chain, task_context<ReturnType> & ctx)
            : chain_(&chain)
            , ctx_(&ctx)
        {
        }

        auto execute() -> void override
        {
            chain_->execute_top(*ctx_);
        }
    };

    class task_runner
    {
    private:
        static std::unique_ptr<task_runner> global_;
    public:
        virtual auto run(closure && c) -> void = 0;
        virtual auto delay(uint64_t nanos) -> task<void> = 0;

        static auto global() -> task_runner &
        {
            return *global_;
        }
    };

    struct inline_task_runner : task_runner
    {
        auto run(closure && c) -> void override
        {
            c.execute();
        }

        auto delay(uint64_t nanos) -> task<void> override
        {
            return {[nanos]() {
                std::this_thread::sleep_for(std::chrono::nanoseconds(nanos));
            }};
        }
    };

    std::unique_ptr<task_runner> task_runner::global_(std::make_unique<inline_task_runner>());
    
    template<class ReturnType>
    auto task<ReturnType>::operator()() -> ReturnType
    {
        return (*this)(task_runner::global());
    }

    template<class ReturnType>
    auto task<ReturnType>::operator()(task_runner & runner) -> ReturnType
    {
        auto ctx = task_context<ReturnType>{};
        auto p = &chain_;
        while (p)
        {
            runner.run(task_closure<ReturnType>{*p, ctx});
            p = p->next();
        }
        if constexpr (not std::is_void_v<ReturnType>)
        {
            return ctx.release_result();
        }
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
