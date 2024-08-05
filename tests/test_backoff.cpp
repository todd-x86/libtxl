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
};

uint64_t sleep_counter::total_nanos_ = 0;
int sleep_counter::num_sleeps_ = 0;

TXL_UNIT_TEST(backoff)
{
    txl::scaled_backoff<sleep_counter> backoff{};

    backoff.wait();
    assert(sleep_counter::num_sleeps_ == 1);
    assert(sleep_counter::total_nanos_ == 15);

    backoff.wait();
    assert(sleep_counter::num_sleeps_ == 2);
    assert(sleep_counter::total_nanos_ == (15) + (15 << 1));
    
    backoff.wait();
    assert(sleep_counter::num_sleeps_ == 3);
    assert(sleep_counter::total_nanos_ == (15) + (15 << 1) + (15 << 2));
    
    backoff.reset();
    backoff.wait();
    assert(sleep_counter::num_sleeps_ == 4);
    assert(sleep_counter::total_nanos_ == (15) + (15 << 1) + (15 << 2) + (15));
}

TXL_RUN_TESTS()
