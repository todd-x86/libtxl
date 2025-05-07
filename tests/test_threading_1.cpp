#include <txl/unit_test.h>
#include <iostream>
#include <atomic>
#include <array>
#include <txl/threading.h>
#include <thread>
#include <vector>

class awaiter
{
private:
    std::mutex m_;
    std::condition_variable c_;
public:
    auto wait()
    {
        auto lock = std::unique_lock<std::mutex>{m_};
        c_.wait(lock);
    }

    auto notify_all()
    {
        c_.notify_all();
    }
};

struct work_thread
{
    awaiter & wait_;
    size_t n_;
    std::thread w_;
    size_t counted_ = 0;

    work_thread(awaiter & wait, size_t n)
        : wait_{wait}
        , n_{n}
    {
    }

    auto start() -> void
    {
        w_ = std::thread{[this]() { execute(); }};
    }

    auto join() -> void
    {
        w_.join();
    }

    auto execute() -> void
    {
        while (counted_ < n_)
        {
            wait_.wait();
            ++counted_;
        }
    }
};

TXL_UNIT_TEST(threading_test)
{
    awaiter w{};
    std::vector<work_thread> workers{};
    for (auto i = 0; i < 2; ++i)
    {
        workers.emplace_back(w, 1000);
    }
    for (auto & worker : workers)
    {
        worker.start();
    }
    for (auto i = 0; i < 1000; ++i)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds{1});
        w.notify_all();
    }
    
    for (auto & worker : workers)
    {
        worker.join();
        assert_equal(worker.counted_, 1000);
    }
}

TXL_UNIT_TEST(done)
{
}

TXL_RUN_TESTS()
