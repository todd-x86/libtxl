#include <txl/unit_test.h>
#include <txl/backoff.h>

#include <cstdint>

struct sleep_counter
{
    static uint64_t total_nanos_;
    static int num_sleeps_;

    auto operator()(uint64_t nanos)
    {
        total_nanos_ += nanos;
        ++num_sleeps_;
    }

    static auto reset_nanos()
    {
        total_nanos_ = 0;
    }
};

uint64_t sleep_counter::total_nanos_ = 0;
int sleep_counter::num_sleeps_ = 0;

TXL_UNIT_TEST(backoff)
{
    txl::scaled_backoff<sleep_counter> backoff{};

    backoff.wait();
    assert_equal(sleep_counter::num_sleeps_, 1);
    assert_equal(sleep_counter::total_nanos_, 15);
    sleep_counter::reset_nanos();

    backoff.wait();
    assert_equal(sleep_counter::num_sleeps_, 2);
    assert_equal(sleep_counter::total_nanos_, (15 << 1) + 1);
    sleep_counter::reset_nanos();
    
    backoff.wait();
    assert_equal(sleep_counter::num_sleeps_, 3);
    assert_equal(sleep_counter::total_nanos_, (((15 << 1) + 1) << 1) + 1);
    sleep_counter::reset_nanos();
    
    backoff.reset();
    backoff.wait();
    assert_equal(sleep_counter::num_sleeps_, 4);
    assert_equal(sleep_counter::total_nanos_, 15);
}

TXL_RUN_TESTS()
