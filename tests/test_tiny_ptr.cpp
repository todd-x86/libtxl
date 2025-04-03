#include <txl/unit_test.h>
#include <txl/tiny_ptr.h>

#include <memory>
#include <string>

struct custom_class
{
    std::string name = "Father Dougal McGuire";
    double age = 26.1;
    bool switches_and_stuff = false;
};

TXL_UNIT_TEST(tiny_ptrs)
{
    using txl::to_tiny_ptr;
    using txl::from_tiny_ptr;

    auto a = std::make_unique<int>(123456);
    auto b = std::make_unique<double>(1.23456);
    auto c = std::make_unique<std::string>("That's fuppin' it! I'm callin' the fuppin' man!");
    auto d = std::make_unique<custom_class>();

    auto ta = to_tiny_ptr(a.get());
    auto tb = to_tiny_ptr(b.get());
    auto tc = to_tiny_ptr(c.get());
    auto td = to_tiny_ptr(d.get());

    assert_equal(from_tiny_ptr(ta), a.get());
    assert_equal(from_tiny_ptr(tb), b.get());
    assert_equal(from_tiny_ptr(tc), c.get());
    assert_equal(from_tiny_ptr(td), d.get());

    assert_equal(*from_tiny_ptr(ta), 123456);
    assert_equal(*from_tiny_ptr(tb), 1.23456);
    assert_equal(*from_tiny_ptr(tc), "That's fuppin' it! I'm callin' the fuppin' man!");
}

TXL_RUN_TESTS()
