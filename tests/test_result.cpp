#include <txl/result.h>
#include <txl/unit_test.h>

#include <stdexcept>
#include <string>

struct error_context final
{
    using error_type = std::string;
    using exception_type = std::runtime_error;
};

// Result where std::string is both the value and the error type
using test_result = txl::result<std::string, error_context>;

auto error_or_value() -> test_result
{
    return {"Hello"};
}

auto as_error() -> test_result
{
    return txl::as_error("Hello");
}

auto as_result() -> test_result
{
    return txl::as_result("Hello");
}

TXL_UNIT_TEST(same_value_error_types)
{
    test_result t{};
    t = error_or_value();
    assert_equal(t.or_throw(), "Hello");
    t = as_error();
    assert_throws<std::runtime_error>([&]() {
        t.or_throw();
    });
    t = as_result();
    assert_equal(t.or_throw(), "Hello");
    t = txl::as_result("World");
    assert_equal(t.or_throw(), "World");
}

TXL_RUN_TESTS()
