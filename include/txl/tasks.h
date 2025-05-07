#pragma once

#include <txl/threading.h>
#include <txl/storage_union.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <exception>
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
        awaiter & awaiter_;
    public:
        future(awaiter & a)
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
        storage_union<ReturnType, std::exception_ptr> res_{};
        awaiter awaiter_{};

        auto notify_awaiters() -> void
        {
            awaiter_.notify_all();
        }
    public:
        promise() = default;
        promise(promise && p)
            : res_(std::move(p.res_))
            , awaiter_(std::move(p.awaiter_))
        {
        }

        auto reset() -> void
        {
            res_.reset();
            awaiter_.reset();
        }
        
        auto operator=(promise && p) -> promise &
        {
            if (this != &p)
            {
                res_ = std::move(p.res_);
                awaiter_ = std::move(p.awaiter_);
            }
            return *this;
        }

        auto get_future() -> future<ReturnType>
        {
            return {awaiter_};
        }

        auto release_value() -> ReturnType &&
        {
            return std::move(res_.template ref<ReturnType>());
        }

        auto get_value() const -> ReturnType const &
        {
            return res_.template ref<ReturnType>();
        }

        auto has_value() const -> bool { return res_.template has<ReturnType>(); }
        auto has_exception() const -> bool { return res_.template has<std::exception_ptr>(); }
        auto get_exception() const -> std::exception_ptr { return res_.template get<std::exception_ptr>(); }
        
        auto rethrow_if_exception() const -> void
        {
            if (has_exception())
            {
                std::rethrow_exception(get_exception());
            }
        }
        
        auto set_exception(std::exception_ptr && p, bool notify = true) -> void
        {
            res_ = std::move(p);
            if (notify)
            {
                notify_awaiters();
            }
        }

        auto set_value(ReturnType && val, bool notify = true) -> void
        {
            res_ = std::move(val);
            if (notify)
            {
                notify_awaiters();
            }
        }

        auto notify_all() -> void
        {
            if (not res_.empty())
            {
                notify_awaiters();
            }
        }
    };
    
    template<>
    class promise<void>
    {
    private:
        awaiter awaiter_{};
        // TODO: atomic_bool
        storage_union<std::exception_ptr, bool> res_{};

        auto notify_awaiters() -> void
        {
            awaiter_.notify_all();
        }
    public:
        promise() = default;
        promise(promise && p)
            : awaiter_(std::move(p.awaiter_))
        {
            std::swap(res_, p.res_);
        }
        
        auto reset() -> void
        {
            res_ = false;
            awaiter_.reset();
        }

        auto operator=(promise && p) -> promise &
        {
            if (this != &p)
            {
                awaiter_ = std::move(p.awaiter_);
                std::swap(res_, p.res_);
            }
            return *this;
        }
        
        auto get_future() -> future<void>
        {
            return {awaiter_};
        }

        auto has_value() const -> bool { return res_.template has<bool>(); }
        auto has_exception() const -> bool { return res_.has<std::exception_ptr>(); }
        auto get_exception() const -> std::exception_ptr { return res_.get<std::exception_ptr>(); }

        auto rethrow_if_exception() const -> void
        {
            if (has_exception())
            {
                std::rethrow_exception(get_exception());
            }
        }
        
        auto set_exception(std::exception_ptr && p, bool notify = true) -> void
        {
            res_ = std::move(p);
            if (notify)
            {
                notify_awaiters();
            }
        }
        
        auto set_value(bool notify = true) -> void
        {
            res_ = true;
            if (notify)
            {
                notify_awaiters();
            }
        }
        
        auto notify_all() -> void
        {
            if (not res_.empty())
            {
                notify_awaiters();
            }
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
            prom_->set_value(std::move(v), false);
        }

        auto set_exception(std::exception_ptr && p) -> void
        {
            prom_->set_exception(std::move(p), false);
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
        
        auto set_result() -> void
        {
            prom_->set_value(false);
        }

        auto set_exception(std::exception_ptr && p) -> void
        {
            prom_->set_exception(std::move(p), false);
        }
    };

    template<class ReturnType>
    struct task_function
    {
        virtual ~task_function() = default;
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

                // By nature of a non-returning task, we must acknowledge
                // that it can be empty and thus if we execute it we must
                // enforce the promise.
                ctx.set_result();
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
        std::unique_ptr<task_function<ReturnType>> work_{nullptr};
        std::unique_ptr<task_chain> next_{nullptr};
    public:
        task_chain() = default;

        task_chain(task_chain && tc)
            : work_(std::move(tc.work_))
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

            work_->execute(ctx);
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
        promise<ReturnType> prom_;
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

        task_core(promise<ReturnType> && p)
            : prom_(std::move(p))
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
            return prom_;
        }

        auto empty() const -> bool
        {
            return chain_.empty();
        }

        auto wait() -> void
        {
            prom_.get_future().wait();
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

        auto run(task_runner & runner) -> future<ReturnType>;

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

        auto run(task_runner & runner) -> future<void>;
        
        auto operator()() -> void;

        auto operator()(task_runner & runner) -> void;
    };

    struct closure : thread_pool_work
    {
        virtual auto move() const -> std::unique_ptr<closure> = 0;
    };

    template<class ReturnType>
    class task_closure : public closure
    {
    private:
        promise<ReturnType> * prom_;
        task_chain<ReturnType> * chain_;
        task_context<ReturnType> ctx_;
    public:
        task_closure(promise<ReturnType> & prom, task_chain<ReturnType> & chain)
            : prom_(&prom)
            , chain_(&chain)
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
            ctx_ = task_context<ReturnType>{*prom_};
            try
            {
                chain_->execute_top(ctx_);
            }
            catch (...)
            {
                ctx_.set_exception(std::current_exception()); 
            }
        }

        auto next() -> bool override
        {
            if (chain_)
            {
                // Move promise forward
                chain_ = chain_->next();
                
                if (chain_)
                {
                    return true;
                }
            }

            return false;
        }

        auto complete() -> void override
        {
            if (not prom_->has_value() and not prom_->has_exception())
            {
                prom_->set_exception(std::make_exception_ptr( std::runtime_error{"empty task did not return a result"} ));
            }
            prom_->notify_all();
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
            c.complete();
        }

        auto delay(uint64_t nanos) -> task<void> override
        {
            return {[nanos]() {
                std::this_thread::sleep_for(std::chrono::nanoseconds(nanos));
            }};
        }
    };

    std::unique_ptr<task_runner> task_runner::global_(std::make_unique<inline_task_runner>());

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
    auto task<ReturnType>::run(task_runner & runner) -> future<ReturnType>
    {
        this->prom_.reset();
        runner.run(task_closure<ReturnType>{this->prom_, this->chain_});
        return this->prom_.get_future();
    }
    
    template<class ReturnType>
    auto task<ReturnType>::operator()() -> ReturnType &&
    {
        return (*this)(task_runner::global());
    }
    
    auto task<void>::run(task_runner & runner) -> future<void>
    {
        this->prom_.reset();
        runner.run(task_closure<void>{this->prom_, this->chain_});
        return this->prom_.get_future();
    }
    
    auto task<void>::operator()() -> void
    {
        (*this)(task_runner::global());
    }

    template<class ReturnType>
    auto task<ReturnType>::operator()(task_runner & runner) -> ReturnType &&
    {
        run(runner).wait();
        this->prom_.rethrow_if_exception();
        return this->prom_.release_value();
    }
    
    auto task<void>::operator()(task_runner & runner) -> void
    {
        run(runner).wait();
        this->prom_.rethrow_if_exception();
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
