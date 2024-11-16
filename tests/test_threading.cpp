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

TXL_RUN_TESTS()
