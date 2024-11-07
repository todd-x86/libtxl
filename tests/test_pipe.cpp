#include <txl/pipe.h>
#include <txl/unit_test.h>

#include <string_view>

TXL_UNIT_TEST(TestOpen)
{
    auto c = txl::pipe_connector{};
    assert_false(c.input().is_open());
    assert_false(c.output().is_open());

    c.open();

    assert_true(c.input().is_open());
    assert_true(c.output().is_open());
}

TXL_UNIT_TEST(TestReadWrite)
{
    auto c = txl::pipe_connector{};
    c.open();

    assert_true(c.input().is_open());
    assert_true(c.output().is_open());

    {
        auto res = c.output().write(std::string_view{"Hello World"});
        assert_equal(res.size(), 11);
    }
    {
        char buf[12] = {0};
        auto res = c.input().read(buf);
        assert_equal(res.size(), 11);
        assert_equal(std::string_view{"Hello World"}, std::string_view{buf});
    }
}

TXL_RUN_TESTS()
