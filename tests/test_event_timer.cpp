#include <txl/event_poller.h>
#include <txl/event_timer.h>
#include <txl/unit_test.h>

#include <chrono>

TXL_UNIT_TEST(TestEventTimer)
{
    auto p = txl::event_poller{};
    auto t = txl::event_timer{};
    auto ev = txl::event_array<4>{};

    auto t1 = std::chrono::steady_clock::now() + std::chrono::milliseconds{50};

    p.open();
    t.open(txl::event_timer::steady_clock);
    t.set_time(t1);

    p.add(t.fd(), txl::event_type::in | txl::event_type::out);
    auto num_polled = p.poll(ev);
    assert_equal(num_polled, 1);
    assert_equal(ev[0].fd(), t.fd());

    p.close();
}

TXL_RUN_TESTS()
