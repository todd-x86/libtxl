#include <txl/unit_test.h>
#include <txl/virtual_ptr.h>

int resources_created_ = 0;
int resources_deleted_ = 0;

struct resource
{
    int value;

    resource(int v)
        : value(v)
    {
        ++resources_created_;
        std::cout << "Creating resource (" << value << ")..." << std::endl;
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
    resources_deleted_ = 0;
    assert_equal(resources_deleted_, 0);
    {
        auto o = owner{txl::make_heap_ptr<resource>(124)};
    }
    assert_equal(resources_deleted_, 1);
    {
        auto o = owner{&r};
    }
    assert_equal(resources_deleted_, 1);
}

TXL_UNIT_TEST(virtual_ptr_copy)
{
    resources_created_ = 0;
    resources_deleted_ = 0;
    
    {
        txl::heap_ptr<resource> vr = txl::make_heap_ptr<resource>(200);
        assert_equal(resources_created_, 1);
        assert_equal(resources_deleted_, 0);
    }
    assert_equal(resources_created_, 1);
    assert_equal(resources_deleted_, 1);

    resources_created_ = 0;
    resources_deleted_ = 0;
    
    {
        txl::virtual_ptr<resource> vr = txl::make_heap_ptr<resource>(201);
        assert_equal(resources_created_, 1);
        assert_equal(resources_deleted_, 0);
    }
    assert_equal(resources_created_, 1);
    assert_equal(resources_deleted_, 1);
}

TXL_RUN_TESTS()
