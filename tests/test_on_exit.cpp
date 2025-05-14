#include <txl/unit_test.h>
#include <txl/on_exit.h>

TXL_UNIT_TEST(on_exit_basic)
{
    auto exited = 0;
    {
        txl::on_exit _{[&] {
            ++exited;
        }};
        assert_equal(exited, 0);
    }
    assert_equal(exited, 1);
}

TXL_UNIT_TEST(on_exit_nullptr)
{
    assert_no_throw([&] {
        txl::on_exit _{nullptr};
    });
}

TXL_RUN_TESTS()
