#pragma once

#include <mutex>
#include <memory>
#include <queue>

namespace txl
{
    template<class Value>
    class resource final
    {
        friend class resource_pool<Value>;
    private:
        std::unique_ptr<Value> data_;

        resource(std::unique_ptr<Value> && data)
            : data_(std::move(data))
        {
        }
    public:
        resource() = default;
        resource(resource const &) = delete;
        resource(resource &&) = default;

        resource & operator=(resource const &) = delete;
        resource & operator=(resource &&) = default;

        Value & operator*() { return *data_; }
        Value * operator->() { return data_.operator->(); }
        Value const & operator*() const { return *data_; }
        Value const * operator->() const { return data_.operator->(); }

        Value * get() { return data_.get(); }
        Value const * get() const { return data_.get(); }

        bool empty() const { return !static_cast<bool>(data_); }
        operator bool const { return static_cast<bool>(data_); }
    };

    template<class Value>
    class resource_pool
    {
    private:
        std::queue<resource<Value>> ready_;
        std::mutex lock_;
    public:
        resource_pool(size_t init_size = 0)
        {
            for (size_t i = 0; i != init_size; ++i)
            {
                ready_.emplace(std::make_unique<Value>());
            }
        }

        resource<Value> get()
        {
            std::unique_lock<std::mutex> lock(lock_);

            if (ready_.size() != 0)
            {
                auto e = std::move(ready_.front());
                ready_.pop();
                return e;
            }
            else
            {
                return resource<Value>(std::make_unique<Value>());
            }
        }

        void put(resource<Value> && res)
        {
            std::unique_lock<std::mutex> lock(lock_);

            if (!res.empty())
            {
                queue_.emplace(std::move(res));
            }
        }
    };
}
