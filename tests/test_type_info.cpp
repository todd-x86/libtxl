#include <txl/unit_test.h>
#include <txl/type_info.h>
#include <string>

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
    auto t4 = txl::get_type_info<std::string>();

    assert(t1.str() == "class_a");
    assert(t2.str() == "class_b<int>");
    assert(t3.str() == "int");
    assert(t4.str() == "std::__cxx11::basic_string<char>");
}

TXL_RUN_TESTS()
