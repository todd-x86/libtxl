#include <txl/unit_test.h>
#include <txl/string_builder.h>

TXL_UNIT_TEST(string_builder)
{
    txl::string_builder sb{4};

    sb.append("hi");
    sb.append("hi");

    assert_equal(sb.to_string(), "hihi");

    sb.append(" hello");
    sb.append(" - goodbye");

    assert_equal(sb.to_string(), "hihi hello - goodbye");
}

TXL_UNIT_TEST(string_builder_clear)
{
    txl::string_builder sb{16};

    sb.append("Here's your secret message");
    sb.append("");

    assert_equal(sb.to_string(), "Here's your secret message");

    sb.clear();
    assert_equal(sb.to_string(), "");

    sb.append("R");
    sb.append("3");
    sb.append("D");
    sb.append("R");
    sb.append("U");
    sb.append("M");
    assert_equal(sb.to_string(), "R3DRUM");
}

TXL_RUN_TESTS()
