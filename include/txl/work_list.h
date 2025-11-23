#include <optional>
#include <memory>
#include <deque>
#include <vector>

namespace txl
{
    struct work_status final
    {
        bool executed = false;
        bool success = true;
        bool complete = true;

        work_status(bool executed, bool success, bool complete)
            : executed{executed}
            , success{success}
            , complete{complete}
        {}
    };

    struct work_unit
    {
        virtual ~work_unit() = default;
        virtual auto execute() -> work_status = 0;
    };

    using work_unit_ptr = std::unique_ptr<work_unit>;

    struct worker : work_unit
    {
        virtual auto add(work_unit_ptr & work) -> bool = 0;
    };

    using worker_ptr = std::unique_ptr<worker>;

    namespace work_units
    {
        template<class Func>
        class lambda_work_unit : public ::txl::work_unit
        {
        private:
            Func work_;
            size_t retries_;
        public:
            lambda_work_unit(Func && func, size_t retries)
                : work_{std::move(func)}
                , retries_{retries}
            {}

            auto execute() -> work_status override
            {
                try
                {
                    work_();
                }
                catch (...)
                {
                    auto retry = retries_ > 0;
                    if (retry)
                    {
                        --retries_;
                    }
                    return {true, false, not retry};    
                }
                return {true, true, true};
            }
        };

        template<class Func>
        inline auto lambda(Func && func, size_t retries)
        {
            return std::make_unique<lambda_work_unit<decltype(func)>>(std::move(func), retries);
        }
    }

    class work_list final
    {
    private:
        std::deque<work_unit_ptr> work_;
    public:
        auto push_front(work_unit_ptr && ptr) -> void
        {
            work_.emplace_front(std::move(ptr));
        }

        auto push_back(work_unit_ptr && ptr) -> void
        {
            work_.emplace_back(std::move(ptr));
        }

        template<class WorkUnit, class... Args>
        auto emplace_back(Args && ... args) -> void
        {
            work_.emplace_back(std::make_unique<WorkUnit>(std::forward(args)...));
        }

        auto size() const -> size_t { return work_.size(); }
        auto empty() const -> bool { return work_.empty(); }

        auto pop_front() -> std::optional<work_unit_ptr>
        {
            if (work_.empty())
            {
                return {};
            }
            auto work = std::move(work_.front());
            work_.pop_front();
            return std::make_optional(std::move(work));
        }
    };

    class work_chain : public worker
    {
    private:
        work_list work_;
        bool rotate_ = true;
    public:
        work_chain() = default;
        work_chain(work_list && work)
            : work_{std::move(work)}
        {}

        auto add(work_unit_ptr & work) -> bool override
        {
            work_.push_back(std::move(work));
            return true;
        }

        auto execute() -> work_status override
        {
            auto work = work_.pop_front();
            if (not work.has_value())
            {
                return {false, true, true};
            }
            auto status = (*work)->execute();
            if (not status.complete)
            {
                if (rotate_)
                {
                    work_.push_back(std::move(*work));
                }
                else
                {
                    work_.push_front(std::move(*work));
                }
            }
            return {status.executed, status.success, work_.empty()};
        }
    };

    class inline_worker : public worker
    {
    private:
        work_unit_ptr curr_;
    public:
        auto add(work_unit_ptr & work) -> bool override
        {
            if (curr_)
            {
                return false;
            }
            curr_ = std::move(work);
            return true;
        }

        auto execute() -> work_status override
        {
            if (curr_ == nullptr)
            {
                return work_status(false, true, true);
            }
            auto status = curr_->execute();
            if (status.complete)
            {
                curr_ = nullptr;
            }

            return status;
        }
    };

    class work_distributor final
    {
    private:
        struct worker_state final
        {
            std::unique_ptr<worker> worker_;
            bool busy_ = false;

            worker_state(std::unique_ptr<worker> && w)
                : worker_{std::move(w)}
            {}

            auto set_busy()
            {
                auto was_busy = busy_;
                busy_ = true;
                return not was_busy;
            }

            auto set_not_busy()
            {
                auto busy = busy_;
                busy_ = false;
                return busy;
            }
        };
        std::vector<worker_state> workers_;
        size_t num_busy_ = 0;
    public:
        template<class Worker, class... Args>
        auto create_worker(Args && ... args)
        {
            workers_.emplace_back(std::make_unique<Worker>(std::forward<Args>(args)...));
        }

        auto run(work_list & work) -> void
        {
            if (workers_.empty())
            {
                return;
            }
            auto index = 0;
            auto unit = work.pop_front();
            while (unit.has_value() or num_busy_ != 0)
            {
                auto & state = workers_[index];
                auto added = state.worker_->add(*unit);
                if (added)
                {
                    unit = work.pop_front();
                }
                
                auto exec = state.worker_->execute();
                if (exec.complete)
                {
                    if (state.set_not_busy())
                    {
                        --num_busy_;
                    }
                }
                else if (not exec.complete and state.set_busy())
                {
                    ++num_busy_;
                }
                ++index;
                if (index == workers_.size())
                {
                    index = 0;
                }
            }
        }
    };
}
