#include <txl/unit_test.h>
#include <txl/virtual_ptr.h>

int resources_deleted_ = 0;

struct resource
{
    int value;

    resource(int v)
        : value(v)
    {
    }

    ~resource()
    {
        ++resources_deleted_;
        std::cout << "Deleting resource (" << value << ")..." << std::endl;
    }
};

struct owner
{
    txl::virtual_ptr<resource> res_;
};

resource r(123);

TXL_UNIT_TEST(virtual_ptr)
{
    assert_equal(resources_deleted_, 0);
    {
        auto o = owner{txl::make_heap_ptr<resource>(123)};
    }
    assert_equal(resources_deleted_, 1);
    {
        auto o = owner{&r};
    }
    assert_equal(resources_deleted_, 1);
}

TXL_RUN_TESTS()
