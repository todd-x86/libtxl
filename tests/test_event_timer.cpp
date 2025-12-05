#include <txl/event_poller.h>
#include <txl/event_timer.h>
#include <txl/unit_test.h>

#include <chrono>

TXL_UNIT_TEST(steady_clock_timer)
{
    auto p = txl::event_poller{};
    auto t = txl::event_timer{};
    auto ev = txl::event_array<4>{};

    auto t1 = std::chrono::steady_clock::now() + std::chrono::milliseconds{10};

    p.open().or_throw();
    t.open(txl::event_timer::steady_clock).or_throw();
    t.set_time(t1).or_throw();

    p.add(t.fd(), txl::event_type::in | txl::event_type::out).or_throw();
    auto num_polled = p.poll(ev, std::chrono::milliseconds{10}).or_throw();
    assert_equal(num_polled, 1);
    assert_equal(ev[0].fd(), t.fd());
    assert(std::chrono::steady_clock::now() >= t1);

    p.close().or_throw();
}

TXL_UNIT_TEST(system_clock_timer)
{
    auto p = txl::event_poller{};
    auto t = txl::event_timer{};
    auto ev = txl::event_array<4>{};

    auto t1 = std::chrono::system_clock::now() + std::chrono::milliseconds{10};

    p.open().or_throw();
    t.open(txl::event_timer::system_clock).or_throw();
    t.set_time(t1).or_throw();

    p.add(t.fd(), txl::event_type::in | txl::event_type::out).or_throw();
    auto num_polled = p.poll(ev, std::chrono::milliseconds{20}).or_throw();
    assert(std::chrono::system_clock::now() >= t1);
    assert_equal(num_polled, 1);
    assert_equal(ev[0].fd(), t.fd());

    p.close().or_throw();
}

TXL_UNIT_TEST(looping)
{
    auto p = txl::event_poller{};
    auto t = txl::event_timer{};
    auto ev = txl::event_array<4>{};

    auto t1 = std::chrono::system_clock::now() + std::chrono::milliseconds{10};

    p.open().or_throw();
    t.open(txl::event_timer::system_clock).or_throw();
    t.set_time(t1, std::chrono::milliseconds{10}).or_throw();

    p.add(t.fd(), txl::event_type::in | txl::event_type::out).or_throw();
    for (auto i = 0; i < 3; ++i)
    {
        auto num_polled = p.poll(ev, std::chrono::milliseconds{20}).or_throw();
        assert(std::chrono::system_clock::now() >= t1);
        t1 += std::chrono::milliseconds{10};
        assert_equal(num_polled, 1);
        assert_equal(ev[0].fd(), t.fd());
        t.read().or_throw();
    }
    p.close().or_throw();
}

TXL_RUN_TESTS()
