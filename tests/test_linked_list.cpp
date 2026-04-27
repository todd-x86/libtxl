#include <txl/unit_test.h>
#include <txl/linked_list.h>
#include <txl/threading.h>
#include <txl/fixed_string.h>

#include <sstream>

TXL_UNIT_TEST(atomic_linked_list_simple)
{
    auto l = txl::atomic_linked_list<int>{};
    assert_true(l.empty());
}

TXL_UNIT_TEST(atomic_linked_list_push_pop)
{
    auto l = txl::atomic_linked_list<int>{};
    l.emplace_front(1);
    l.emplace_front(2);
    l.emplace_front(3);
    assert_false(l.empty());
    assert_equal(*l.pop_and_release_front(), 3);
    assert_equal(*l.pop_and_release_front(), 2);
    assert_equal(*l.pop_and_release_front(), 1);
    assert_true(l.empty());
}

static size_t num_deletes = 0;

struct delete_me final
{
    bool can_delete_ = true;

    delete_me() = default;

    delete_me(delete_me && d)
    {
        d.can_delete_ = false;
    }

    ~delete_me()
    {
        if (can_delete_)
        {
            ++num_deletes;
        }
    }
};

TXL_UNIT_TEST(atomic_linked_list_free)
{
    auto l = txl::atomic_linked_list<delete_me>{};
    assert_equal(num_deletes, 0);
    l.emplace_front();
    l.emplace_front();
    l.emplace_front();
    assert_equal(num_deletes, 0);
    assert_false(l.empty());
    l.pop_and_release_front();
    assert_equal(num_deletes, 1);
    l.pop_and_release_front();
    assert_equal(num_deletes, 2);
    l.pop_and_release_front();
    assert_equal(num_deletes, 3);
    l.pop_and_release_front();
    assert_equal(num_deletes, 3);
    assert_true(l.empty());
}

TXL_UNIT_TEST(atomic_linked_list_move)
{
    auto l = txl::atomic_linked_list<std::string>{};
    l.emplace_front("Hello World No Small String Optimization Here");
    assert_false(l.empty());
    auto s = std::move(*l.pop_and_release_front());
    assert_true(l.empty());
    assert_equal(s, "Hello World No Small String Optimization Here");
}

TXL_UNIT_TEST(atomic_linked_list_add_lots)
{
    auto l = txl::atomic_linked_list<std::string>{};
    auto ss = std::ostringstream{};
    for (auto i = 0; i < 10000; ++i)
    {
        ss.str("");
        ss << "Hello from the following number: " << i;
        l.emplace_front(ss.str());
    }
    
    for (auto i = 9999; i >= 0; --i)
    {
        ss.str("");
        ss << "Hello from the following number: " << i;
        assert_equal(*l.pop_and_release_front(), ss.str());
    }
    assert_true(l.empty());
}

TXL_UNIT_TEST_N(atomic_linked_list_concurrency, 100)
{
    auto l = txl::atomic_linked_list<txl::fixed_string<20>>{};
    auto a = txl::awaiter{};
    std::atomic<int> counter = 0;

    auto add_n = [&](int n) {
        return [&,n]() {
            a.wait();
            for (auto i = 0; i < n; ++i)
            {
                l.emplace_front("Complicated");
            }
        };
    };

    auto pop_n = [&](int n) {
        return [&,n]() {
            a.wait();
            for (auto i = 0; i < n;)
            {
                auto el = l.pop_and_release_front();
                if (el)
                {
                    ++i;
                    if (*el == "Complicated")
                    {
                        counter.fetch_add(1, std::memory_order_release);
                    }
                }
            }
        };
    };

    std::thread t1{add_n(100)}, t2{pop_n(100)}, t3{add_n(200)}, t4{pop_n(190)};
    a.set();
    t1.join();
    t2.join();
    t3.join();
    //assert_equal(l.num_inserts(), 100+200);
    //assert_equal(l.num_pops(), 100);
    t4.join();
    assert_equal(l.num_inserts(), 100+200);
    assert_equal(l.num_pops(), 100+190);
    
    //assert_equal(counter.load(std::memory_order_acquire), 240);
}

TXL_UNIT_TEST_N(atomic_linked_list_thread_safety, 100)
{
    auto l = txl::atomic_linked_list<int>{};
    std::atomic<int> total_pushed = 0;
    std::atomic<int> total_popped = 0;

    constexpr int num_producers = 3;
    constexpr int num_consumers = 3;
    constexpr int items_per_producer = 500;
    constexpr int total_items = num_producers * items_per_producer;

    auto producer = [&](int id) {
        return [&, id]() {
            for (auto i = 0; i < items_per_producer; ++i)
            {
                l.emplace_front(id * items_per_producer + i);
                total_pushed.fetch_add(1, std::memory_order_release);
            }
        };
    };

    auto consumer = [&]() {
        while (total_popped.load(std::memory_order_acquire) < total_items)
        {
            auto value = l.pop_and_release_front();
            if (value)
            {
                total_popped.fetch_add(1, std::memory_order_release);
            }
        }
    };

    std::thread producers[num_producers];
    std::thread consumers[num_consumers];

    for (auto i = 0; i < num_producers; ++i)
    {
        producers[i] = std::thread(producer(i));
    }

    for (auto i = 0; i < num_consumers; ++i)
    {
        consumers[i] = std::thread(consumer);
    }

    for (auto i = 0; i < num_producers; ++i)
    {
        producers[i].join();
    }

    for (auto i = 0; i < num_consumers; ++i)
    {
        consumers[i].join();
    }

    assert_equal(total_pushed.load(), total_items);
    assert_equal(total_popped.load(), total_items);
    assert_equal(l.num_inserts(), total_items);
    assert_equal(l.num_pops(), total_items);
    assert_true(l.empty());
}

TXL_RUN_TESTS()
