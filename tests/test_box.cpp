#include <txl/unit_test.h>
#include <txl/box.h>
#include <vector>
#include <memory>

static size_t created_ = 0, deleted_ = 0;

struct resource
{
    int value;

    resource(int v)
        : value(v)
    {
        ++created_;
    }

    resource(resource const & v)
        : value{v.value}
    {
        ++created_;
    }
    
    resource(resource &&) = default;

    ~resource()
    {
        ++deleted_;
    }
};

TXL_UNIT_TEST(box_ownership)
{
    txl::box<int> x;
    auto p = new int{123};
    x = txl::box<int>{p, true};
    assert_equal(x.get(), p);
}

TXL_UNIT_TEST(box)
{
    created_ = deleted_ = 0;

    assert_equal(created_, 0);
    assert_equal(deleted_, 0);

    std::vector<txl::box<resource>> ptrs{};

    ptrs.emplace_back(std::make_unique<resource>(123));
    assert_equal(created_, 1);
    assert_equal(deleted_, 0);
    
    ptrs.emplace_back(std::make_unique<resource>(234));
    assert_equal(created_, 2);
    assert_equal(deleted_, 0);

    ptrs.clear();
    assert_equal(created_, 2);
    assert_equal(deleted_, 2);
}

TXL_RUN_TESTS()
