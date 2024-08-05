#include <txl/unit_test.h>
#include <txl/bitwise.h>

TXL_UNIT_TEST(is_power_of_two)
{
    assert(txl::is_power_of_two(2));
    assert(not txl::is_power_of_two(1));
    for (auto i = 1; i < 64; ++i) {
        assert(txl::is_power_of_two(static_cast<uint64_t>(1) << i));
    }
    assert(not txl::is_power_of_two(1));
    assert(not txl::is_power_of_two(0));
}

TXL_RUN_TESTS()
