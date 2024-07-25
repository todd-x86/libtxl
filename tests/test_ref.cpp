#include <txl/unit_test.h>
#include <txl/ref.h>

template<class T>
auto set_value(txl::ref<T> value, T new_value)
{
    *value = new_value;
}

TXL_UNIT_TEST(ref_lifetime)
{
    int x = 1;

    set_value({x}, 2);
    assert(x == 2);

    set_value(txl::make_ref(x), 3);
    assert(x == 3);
}

TXL_RUN_TESTS()
