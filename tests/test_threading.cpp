#include <txl/unit_test.h>
#include <txl/threading.h>

TXL_UNIT_TEST(baseline)
{
    std::atomic<int> x = 0;
    int y = 0;
    auto t = txl::threading_unit_test{[&x, &y]() {
        for (auto i = 0; i < 10000; ++i)
        {
            x++;
            y++;
        }
    }};
    t.run();
    assert_equal(x.load(), (1+2+4+8+16)*10000);

    // On a multicore system, this should be different...
    assert_not_equal(y, (1+2+4+8+16)*10000);
}

TXL_UNIT_TEST(awaiter)
{
    std::atomic<size_t> count = 0;
    int num_cycles = 10;

    std::array<txl::awaiter, 3> awaiters{};
    std::array<std::thread, 3> threads{};

    std::atomic<size_t> num_responded = 0;
    txl::awaiter phone_home{};
    for (size_t i = 0; i < threads.size(); ++i)
    {
        awaiters[i] = txl::awaiter{};
        threads[i] = std::thread{[num_cycles,&awaiter=awaiters[i],&count,&num_responded,&phone_home] {
            for (auto i = 0; i < num_cycles; ++i)
            {
                awaiter.wait();
                awaiter.reset();
                count.fetch_add(1, std::memory_order_relaxed);

                num_responded.fetch_add(1, std::memory_order_relaxed);
                phone_home.notify_all();
            }
        }};
    }

    assert_equal(count.load(std::memory_order_relaxed), 0);
    for (auto i = 0; i < num_cycles; ++i)
    {
        num_responded.store(0, std::memory_order_relaxed);
        for (auto & awaiter : awaiters)
        {
            awaiter.notify_all();
        }
        while (num_responded.load(std::memory_order_relaxed) < threads.size())
        {
            phone_home.wait();
            phone_home.reset();
        }
        assert_equal(count.load(std::memory_order_relaxed), threads.size()*(i+1));
    }
    for (auto & thread : threads)
    {
        thread.join();
    }
}

TXL_UNIT_TEST(thread_pool_simple)
{
    auto c = std::atomic_int{0};
    {
        auto tp = txl::thread_pool{2};
        auto added = tp.post_work(txl::make_thread_pool_lambda([&c]() {
            c.fetch_add(1);
        }));
        assert_true(added);
    }
    // We didn't run, so assume no changes
    assert_equal(c.load(), 0);
}

TXL_UNIT_TEST(thread_pool_work)
{
    auto c = std::atomic_int{0};
    {
        auto tp = txl::thread_pool{2};
        auto added = tp.post_work(txl::make_thread_pool_lambda([&c]() {
            c.fetch_add(1);
        }));
        assert_true(added);
        
        added = tp.post_work(txl::make_thread_pool_lambda([&c]() {
            c.fetch_add(1);
        }));
        assert_true(added);
        tp.start_workers();

        tp.wait_for_idle();
    }
    assert_equal(c.load(), 2);
}

TXL_UNIT_TEST(thread_pool_complete_work)
{
    auto c = std::atomic_int{0};
    
    auto tp = txl::thread_pool{2};
    tp.start_workers();

    for (auto i = 0; i < 10000; ++i)
    {
        auto added = tp.post_work(txl::make_thread_pool_lambda([&c]() {
            c.fetch_add(1);
        }));
        assert_true(added);
    }
    
    tp.wait_for_idle();
    tp.stop_workers();
    
    assert_equal(c.load(), 10000);
}

TXL_RUN_TESTS()
