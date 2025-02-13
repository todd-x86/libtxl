#pragma once

#include <functional>
#include <memory>

namespace txl
{
    struct task_context;

    struct task_work_base
    {
        virtual auto execute(task_context &) -> void = 0;
        virtual auto next() -> task_work_base * = 0;
    };

    class task_context
    {
        friend class task_runner_base;
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
    class task_work : public task_work_base
    {
    private:
        std::function<ReturnType(task_context &)> func_;
        std::unique_ptr<task_work_base> next_;
    public:
        task_work(std::function<ReturnType()> && func)
            : func_([func=std::move(func)](task_context &) {
                func();
            })
        {
        }
        
        task_work(std::function<ReturnType(task_context &)> && func)
            : func_(std::move(func))
        {
        }

        auto chain(std::unique_ptr<task_work_base> && next) -> void
        {
            next_ = std::move(next);
        }

        auto execute(task_context & ctx) -> void override
        {
            func_(ctx);
        }
        
        auto next() -> task_work_base * override
        {
            return next_.get();
        }
    };

    struct task_work_runner
    {
        virtual auto run(task_work_base & work, task_context & context) -> void = 0;
    };

    struct inline_task_work_runner : task_work_runner
    {
        auto run(task_work_base & work, task_context & context) -> void override
        {
            work.execute(context);
        }
    };

    class task_base
    {
        friend class task_runner_base;
    protected:
        virtual auto work() const -> task_work_base * = 0;
    };

    class task_runner_base
    {
    protected:
        static auto get_task_work(task_base & task) -> task_work_base *
        {
            return task.work();
        }
    public:
        virtual auto run(task_base & task) -> void = 0;
    };

    class task_runner : public task_runner_base
    {
    private:
        std::unique_ptr<task_work_runner> work_runner_;
    public:
        task_runner(std::unique_ptr<task_work_runner> && runner)
            : work_runner_(std::move(runner))
        {
        }

        auto run(task_base & task) -> void override
        {
            task_context ctx{};

            auto work = get_task_work(task);
            while (work)
            {
                work_runner_->run(*work, ctx);
                work = work->next();
            }
        }
    };

    template<class ReturnType>
    class task : public task_base
    {
    protected:
        virtual auto work() const -> task_work_base *
        {
            return nullptr;
        }
    };

    template<>
    class task<void> : public task_base
    {
    private:
        std::unique_ptr<task_work<void>> work_;
        task_work<void> * tail_ = nullptr;
        
        task(std::unique_ptr<task_work<void>> && w)
            : work_(std::move(w))
            , tail_(work_.get())
        {
        }
    protected:
        auto work() const -> task_work_base * override
        {
            return work_.get();
        }
    public:
        task(std::function<void()> && f)
            : task(std::make_unique<task_work<void>>(std::move(f)))
        {
        }

        auto then(std::function<void()> && f) -> task<void> &&
        {
            auto next = std::make_unique<task_work<void>>(std::move(f));
            tail_->chain(std::move(next));
            tail_ = next.get();
            return std::move(*this);
        }

        auto then(std::function<void(task_context &)> && f) -> task<void> &&
        {
            auto next = std::make_unique<task_work<void>>(std::move(f));
            tail_->chain(std::move(next));
            tail_ = next.get();
            return std::move(*this);
        }
    };

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
