#include <txl/unit_test.h>
#include <txl/option_parser.h>

TXL_UNIT_TEST(options)
{
    std::string x = "";
    bool y = true;

    txl::option_parser opts{};
    opts.add_flag('x', x);
    opts.add_flag('y', y);

    assert_equal(x, "");
    assert_equal(y, false);

    char const * args[4] = {"EXEFILE", "-x", "flippity flip", "-y"};
    opts.parse(sizeof(args) / sizeof(char const *), args);

    assert_equal(opts.get_usage_string("a.out"), "Usage: a.out [ -x <str> ] [ -y ]");

    assert_equal(x, "flippity flip");
    assert_equal(y, true);
}

TXL_RUN_TESTS()
