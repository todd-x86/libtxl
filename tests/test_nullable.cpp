#include <txl/nullable.h>
#include <txl/unit_test.h>
#include <string>

auto call_test(txl::nullable<std::string> name = {}) -> int
{
    OPT_RETURN(name, size());
    return 1;
}

struct test_thing final
{
    int & value_;

    test_thing(int & v)
        : value_{v}
    {
    }

    auto increment(int size)
    {
        value_ += size;
    }

    auto get_value() const
    {
        return value_;
    }
};

auto call_and_invoke(txl::nullable<test_thing> adder = {}) -> void
{
    OPTIONALLY(adder, increment(10));
}


TXL_UNIT_TEST(nullable_simple)
{
    assert_equal(call_test(), 1);
    assert_equal(call_test(std::string{"This is a test"}), 14);
}

TXL_UNIT_TEST(nullable_optionally)
{
    int x = 0;
    call_and_invoke();
    call_and_invoke(test_thing{x});
    assert_equal(x, 10);
}

TXL_RUN_TESTS()
