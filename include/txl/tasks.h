#pragma once

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <thread>

namespace txl
{
    template<class ReturnType>
    class task_context;

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

    struct closure
    {
        virtual auto execute() -> void = 0;
        virtual auto next() -> bool = 0;
    };

    template<class ReturnType>
    class task;

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
        
        task(std::unique_ptr<detail::task_work<ReturnType>> && w)
            : work_(std::move(w))
            , tail_(work_.get())
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
        }
    public:
        task() = default;
        task(std::function<ReturnType()> && f)
            : task(std::make_unique<detail::task_work<ReturnType>>(std::move(f)))
        {
        }

        task(task const &) = delete;

        task(task && t)
            : work_(std::move(t.work_))
        {
            tail_ = t.tail_;
            t.tail_ = nullptr;
        }

        auto operator=(task const &) -> task & = delete;

        auto operator=(task && t) -> task &
        {
            if (this != &t)
            {
                work_ = std::move(t.work_);
                tail_ = t.tail_;
                t.tail_ = nullptr;
            }
            return *this;
        }

        auto empty() const -> bool { return not static_cast<bool>(work_); }

        auto wait() -> void
        {
            if (work_)
            {
                work_->wait();
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
    };

    class inline_task_runner : public task_runner
    {
    public:
        inline_task_runner()
            : task_runner()
        {
            // Mark it completed since we run inline
        }

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


    /*struct thread_pool_task_runner : task_runner
    {
        auto run(closure & c) -> std::future<void> override
        {
            do
            {
                c.execute();
            }
            while (c.next());
        }

        auto delay(int64_t timeout_nanos) -> task<void> override
        {
            return {[timeout_nanos]() {
                std::this_thread::sleep_for(std::chrono::nanoseconds{timeout_nanos});
            }};
        }
    };*/

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
