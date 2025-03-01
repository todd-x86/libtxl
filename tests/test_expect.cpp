#include <txl/unit_test.h>
#include <txl/expect.h>

#include <chrono>
#include <thread>

TXL_UNIT_TEST(expect_at_most)
{
    auto at_most_tripped = false;
    txl::expect::on_at_most([&](auto const & exp, auto const & now) {
        at_most_tripped = true;
    });

    {
        at_most_tripped = false;
        EXPECT_AT_MOST(std::chrono::milliseconds(10));
    }
    assert_false(at_most_tripped);

    {
        at_most_tripped = false;
        EXPECT_AT_MOST(std::chrono::milliseconds(10));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert_true(at_most_tripped);
}

TXL_RUN_TESTS()
