#include <txl/unit_test.h>
#include <txl/type_info.h>
#include <vector>

struct class_a
{
};

template<class T>
struct class_b
{
};

TXL_UNIT_TEST(type_info)
{
    auto t1 = txl::get_type_info<class_a>();
    auto t2 = txl::get_type_info<class_b<int>>();
    auto t3 = txl::get_type_info<int>();
    auto t4 = txl::get_type_info<std::vector<double>>();

    assert_equal(t1.str(), "class_a");
    assert_equal(t2.str(), "class_b<int>");
    assert_equal(t3.str(), "int");
    assert_equal(t4.str(), "std::vector<double>");
}

TXL_RUN_TESTS()
