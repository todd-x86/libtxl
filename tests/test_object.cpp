#include <txl/object.h>
#include <txl/unit_test.h>

#include <memory>

struct concrete_base
{
    int & destructed_;
    
    concrete_base(int & destructed)
        : destructed_(destructed)
    {
    }

    ~concrete_base()
    {
        ++destructed_;
    }
};

struct concrete_derived : concrete_base
{
    int & other_destructed_;

    concrete_derived(int & destructed, int & other_destructed)
        : concrete_base(destructed)
        , other_destructed_(other_destructed)
    {
    }

    ~concrete_derived()
    {
        ++other_destructed_;
    }
};

struct virtual_base : txl::virtual_base
{
    int & destructed_;
    
    virtual_base(int & destructed)
        : destructed_(destructed)
    {
    }

    ~virtual_base()
    {
        ++destructed_;
    }
};

struct virtual_derived : virtual_base
{
    int & other_destructed_;

    virtual_derived(int & destructed, int & other_destructed)
        : virtual_base(destructed)
        , other_destructed_(other_destructed)
    {
    }

    ~virtual_derived()
    {
        ++other_destructed_;
    }
};

TXL_UNIT_TEST(virtual_base)
{
    int c_dtor_1 = 0, c_dtor_2 = 0;
    int v_dtor_1 = 0, v_dtor_2 = 0;

    {
        auto unused = std::unique_ptr<concrete_base>(new concrete_derived(c_dtor_1, c_dtor_2));
    }

    {
        auto unused = std::unique_ptr<virtual_base>(new virtual_derived(v_dtor_1, v_dtor_2));
    }

    // Concrete class has base called incorrectly...
    assert(c_dtor_1 == 1);
    assert(c_dtor_2 == 0);

    // Virtual class is correctly destructed...
    assert(v_dtor_1 == 1);
    assert(v_dtor_2 == 1);
}

TXL_RUN_TESTS()
