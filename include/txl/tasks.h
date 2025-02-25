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

        auto assign(awaiter & a) -> void
        {
            core_ = a.core_;
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

        auto move_from(promise<ReturnType> & p) -> void
        {
            value_ = std::move(p.value_);
            p.awaiter_.assign(awaiter_);
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

        auto execute_top(task_context<ReturnType> & ctx) -> void
        {
            if (not work_)
            {
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
    public:
        task_core() = default;
        
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

        auto operator()() -> ReturnType const &;

        auto operator()(task_runner & runner) -> ReturnType const &;
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
    auto task<ReturnType>::operator()() -> ReturnType const &
    {
        return (*this)(task_runner::global());
    }
    
    auto task<void>::operator()() -> void
    {
        (*this)(task_runner::global());
    }

    template<class ReturnType>
    auto task<ReturnType>::operator()(task_runner & runner) -> ReturnType const &
    {
        auto p = &this->chain_;
        auto ctx = task_context<ReturnType>{};
        promise<ReturnType> * last_prom = nullptr;
        while (p)
        {
            if (last_prom)
            {
                p->get_promise().move_from(*last_prom);
            }
            last_prom = &p->get_promise();
            ctx = task_context<ReturnType>{p->get_promise()};
            runner.run(task_closure<ReturnType>{*p, ctx});
            p = p->next();
        }
        if constexpr (not std::is_void_v<ReturnType>)
        {
            return ctx.result();
        }
    }
    
    auto task<void>::operator()(task_runner & runner) -> void
    {
        auto p = &this->chain_;
        auto ctx = task_context<void>{};
        while (p)
        {
            ctx = task_context<void>{p->get_promise()};
            runner.run(task_closure<void>{*p, ctx});
            p = p->next();
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
