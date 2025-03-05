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
    std::atomic<size_t> next_index = 0;
    std::array<txl::awaiter, 3> awaiters{};
    for (size_t i = 0; i < awaiters.size(); ++i)
    {
        awaiters[i] = txl::awaiter{};
    }

    auto t = txl::threading_unit_test{[&]() {
        auto index = next_index.fetch_add(1);
        if (index == awaiters.size())
        {
            next_index.store(0);
            index = 0;
        }
        auto i = index + 1;
        if (i == awaiters.size())
        {
            i = 0;
        }
        awaiters[i].notify_all();
        awaiters[index].wait();
    }};
    t.set_thread_scale(2, 5);
    t.run();
}

TXL_UNIT_TEST(thread_pool_simple)
{
    auto c = std::atomic_int{0};
    {
        auto tp = txl::thread_pool{2};
        tp.post_work(txl::make_thread_pool_lambda([&c]() {
            c.fetch_add(1);
        }));
    }
    // We didn't run, so assume no changes
    assert_equal(c.load(), 0);
    
    {
        auto tp = txl::thread_pool{2};
        tp.post_work(txl::make_thread_pool_lambda([&c]() {
            c.fetch_add(1);
        }));
        tp.start_workers();
        tp.post_work(txl::make_thread_pool_lambda([&c]() {
            c.fetch_add(1);
        }));
        tp.wait_for_idle();
    }
    assert_equal(c.load(), 2);
}

TXL_RUN_TESTS()
