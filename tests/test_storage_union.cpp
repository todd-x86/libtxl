#include <txl/unit_test.h>
#include <txl/storage_union.h>
#include <string>

static int instances = 0;

struct leak_free final
{
    leak_free()
    {
        ++instances;
    }

    leak_free(leak_free const &)
    {
        ++instances;
    }

    leak_free(leak_free &&)
    {
        ++instances;
    }

    ~leak_free()
    {
        --instances;
    }
};

TXL_UNIT_TEST(storage_union)
{
    auto v = txl::storage_union<int, double, std::string>{};

    v = 1.2;
    assert_equal(static_cast<double>(v), 1.2);
    
    v = -3;
    assert_equal(static_cast<int>(v), -3);
    
    v = "THIS IS A TEST";
    assert_equal(static_cast<std::string>(v), "THIS IS A TEST");
}

TXL_UNIT_TEST(storage_union_leak_free)
{
    auto v = txl::storage_union<int, leak_free, std::string>{};

    v = 12;
    assert_equal(static_cast<int>(v), 12);

    assert_equal(instances, 0);
    
    v = leak_free{};
    assert_equal(instances, 1);
    
    v = 9;
    assert_equal(static_cast<int>(v), 9);
    assert_equal(instances, 0);
}

TXL_RUN_TESTS()
