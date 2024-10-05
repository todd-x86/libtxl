#include <txl/unit_test.h>
#include <txl/ref.h>

template<class T>
auto set_value(txl::ref<T> value, T new_value)
{
    *value = new_value;
}

struct account
{
    int id_;
};

auto get_id(txl::ref<account> acct) -> int
{
    return acct->id_;
}

TXL_UNIT_TEST(ref_lifetime)
{
    int x = 1;

    set_value({x}, 2);
    assert(x == 2);

    set_value(txl::make_ref(x), 3);
    assert(x == 3);
}

TXL_UNIT_TEST(ref_tests)
{
    auto a1 = account{1};
    // l-value
    assert_equal(1, get_id(a1));
    // r-value
    assert_equal(2, get_id(account{2}));
}

TXL_RUN_TESTS()
