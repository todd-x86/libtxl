#pragma once

#include <mutex>
#include <optional>
#include <memory>
#include <queue>

namespace txl
{
    // Forward declaration
    template<class Value, class Factory>
    class resource_pool;

    template<class Value>
    class resource final
    {
        template<class V, class F>
        friend class resource_pool;
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
        operator bool() const { return static_cast<bool>(data_); }
    };

    template<class Value>
    struct resource_factory
    {
        std::unique_ptr<Value> operator()() const
        {
            return std::make_unique<Value>();
        }
    };

    template<class Value, class Factory = resource_factory<Value>>
    class resource_pool
    {
    private:
        Factory factory_;
        std::queue<resource<Value>> ready_;
        std::mutex lock_;
    public:
        resource_pool(Factory && factory, size_t init_size = 0)
            : factory_(std::move(factory))
        {
            for (size_t i = 0; i != init_size; ++i)
            {
                ready_.emplace(resource<Value>(factory_()));
            }
        }

        resource_pool(size_t init_size = 0)
            : resource_pool(Factory(), init_size)
        {
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
                return resource<Value>(factory_());
            }
        }

        void put(resource<Value> && res)
        {
            std::unique_lock<std::mutex> lock(lock_);

            if (!res.empty())
            {
                ready_.emplace(std::move(res));
            }
        }
    };
}
