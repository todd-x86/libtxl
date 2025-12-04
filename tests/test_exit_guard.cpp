#include <txl/unit_test.h>
#include <txl/exit_guard.h>

TXL_UNIT_TEST(exit_guard)
{
    auto exited = false;
    {
        DEFER { exited = true; };
        assert_false(exited);
    }
    assert_true(exited);
}

TXL_UNIT_TEST(multiple_exit_guards)
{
    auto exit = false;
    auto num_defers = 0;
    {
        DEFER {
            exit = true;
            ++num_defers;
        };
        DEFER {
            ++num_defers;
        };
        assert_false(exit);
        assert_equal(num_defers, 0);
    }
    assert_true(exit);
    assert_equal(num_defers, 2);
}

TXL_RUN_TESTS()
