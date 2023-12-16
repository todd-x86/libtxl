#include <txl/unit_test.h>
#include <txl/opaque_ptr.h>

struct resource
{
    int value;

    resource(int v)
        : value(v)
    {
    }

    ~resource()
    {
        std::cout << "Deleting resource (" << value << ")..." << std::endl;
    }
};

resource r(123);

txl::opaque_ptr<resource> test_raw()
{
    return &r;
}

txl::opaque_ptr<resource> test_unique()
{
    return std::make_unique<resource>(456);
}

txl::opaque_ptr<resource> test_shared()
{
    return std::make_shared<resource>(789);
}

void test_opaque_ptr(txl::opaque_ptr<resource> ptr)
{
    std::cout << "tested: " << ptr->value << std::endl;
}

TXL_UNIT_TEST(opaque_ptr)
{
    std::cout << "sizeof(opaque_ptr<resource>) == " << sizeof(txl::opaque_ptr<resource>) << std::endl;
    {
        auto r = test_raw();
        assert(r->value == 123);
    }
    {
        auto u = test_unique();
        assert(u->value == 456);
    }
    {
        auto s = test_shared();
        assert(s->value == 789);
    }
    test_opaque_ptr(test_raw());
    test_opaque_ptr(test_unique());
    test_opaque_ptr(test_shared());
}

TXL_RUN_TESTS()
