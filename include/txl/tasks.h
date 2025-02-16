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
    struct task_context : task_context_base
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
    struct task_context<void> : task_context_base
    {
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

    template<class ReturnType>
    class task_runner;

    template<class ReturnType>
    class task
    {
    private:
        std::unique_ptr<task_work<ReturnType>> work_;
        task_work<ReturnType> * tail_ = nullptr;
        
        task(std::unique_ptr<task_work<ReturnType>> && w)
            : work_(std::move(w))
            , tail_(work_.get())
        {
        }
    public:
        task() = default;
        task(std::function<ReturnType()> && f)
            : task(std::make_unique<task_work<ReturnType>>(std::move(f)))
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
        
        auto then(task<ReturnType> && t) -> task<ReturnType> &&
        {
            if (this == &t or t.empty())
            {
                return std::move(*this);
            }

            if (work_)
            {
                auto new_tail = t.work_->tail();
                assert(tail_ == work_->tail());
                tail_->chain(std::move(t.work_));
                tail_ = new_tail;
            }
            else
            {
                work_ = std::move(t.work_);
                tail_ = work_.get();
            }
            assert(tail_ == work_->tail());
            t.tail_ = nullptr;
            return std::move(*this);
        }

        auto then(std::function<ReturnType()> && f) -> task<ReturnType> &&
        {
            auto next = std::make_unique<task_work<ReturnType>>(std::move(f));
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
            return std::move(*this);
        }

        auto then(std::function<ReturnType(task_context<ReturnType> &)> && f) -> task<ReturnType> &&
        {
            auto next = std::make_unique<task_work<ReturnType>>(std::move(f));
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
            return std::move(*this);
        }
        
        auto operator()() -> ReturnType;
        auto operator()(task_runner<ReturnType> & runner) -> ReturnType;
    };

    /*template<>
    class task<void>
    {
    private:
        std::unique_ptr<task_work<void>> work_;
        task_work<void> * tail_ = nullptr;
        
        task(std::unique_ptr<task_work<void>> && w)
            : work_(std::move(w))
            , tail_(work_.get())
        {
        }
    public:
        task() = default;
        task(std::function<void()> && f)
            : task(std::make_unique<task_work<void>>(std::move(f)))
        {
        }
        
        task(task const &) = delete;

        task(task && t)
            : work_(std::move(t.work_))
        {
            std::swap(tail_, t.tail_);
        }

        auto operator=(task const &) -> task & = delete;

        auto operator=(task && t) -> task &
        {
            if (this != &t and not t.empty())
            {
                work_ = std::move(t.work_);
                std::swap(tail_, t.tail_);
            }
            return *this;
        }

        // TODO: consolidate task<T> and task<void>
        
        auto empty() const -> bool { return not static_cast<bool>(work_); }
        
        auto then(task<void> && t) -> task<void> &&
        {
            if (this == &t or t.empty())
            {
                return std::move(*this);
            }

            if (work_)
            {
                auto new_tail = t.work_->tail();
                tail_->chain(std::move(t.work_));
                tail_ = new_tail;
                t.tail_ = nullptr;
            }
            else
            {
                work_ = std::move(t.work_);
                std::swap(tail_, t.tail_);
            }
            return std::move(*this);
        }

        auto then(std::function<void()> && f) -> task<void> &&
        {
            auto next = std::make_unique<task_work<void>>(std::move(f));
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
            assert(tail_);
            return std::move(*this);
        }

        auto then(std::function<void(task_context<void> &)> && f) -> task<void> &&
        {
            auto next = std::make_unique<task_work<void>>(std::move(f));
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
            return std::move(*this);
        }
        
        auto operator()() -> void;
        auto operator()(task_runner<void> & runner) -> void;
    };*/

    template<class ReturnType>
    class task_runner
    {
    public:
        auto run(task_work<ReturnType> * work) -> ReturnType
        {
            task_context<ReturnType> ctx{};

            while (work)
            {
                if constexpr (std::is_same_v<ReturnType, void>)
                {
                    work->execute(ctx);
                }
                else
                {
                    auto res = work->execute(ctx);
                    ctx.set_result(std::move(res));
                }
                work = work->next();
            }

            if constexpr (not std::is_same_v<ReturnType, void>)
            {
                return std::move(ctx.release_result());
            }
        }

        template<class Rep, class Period>
        auto delay(std::chrono::duration<Rep, Period> const & timeout) -> task<void>
        {
            return {[timeout]() {
                std::this_thread::sleep_for(timeout);
            }};
        }
    };

    template<class ReturnType>
    inline auto task<ReturnType>::operator()() -> ReturnType
    {
        static task_runner<ReturnType> runner{};
        return (*this)(runner);
    }

    template<class ReturnType>
    inline auto task<ReturnType>::operator()(task_runner<ReturnType> & runner) -> ReturnType
    {
        return runner.run(work_.get());
    }

    /*inline auto task<void>::operator()() -> void
    {
        static task_runner<void> runner{};
        (*this)(runner);
    }

    inline auto task<void>::operator()(task_runner<void> & runner) -> void
    {
        runner.run(work_.get());
    }*/

    template<class Rep, class Period>
    inline auto delay(std::chrono::duration<Rep, Period> const & sleep) -> task<void>
    {
        // TODO: consolidate static instances
        static task_runner<void> runner{};
        return runner.delay(sleep);
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
