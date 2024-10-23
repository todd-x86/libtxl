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
    assert_equal(sizeof(txl::storage_union<int, leak_free, std::string>), 40);

    v = 12;
    assert_equal(static_cast<int>(v), 12);

    assert_equal(instances, 0);
    
    v = leak_free{};
    assert_equal(instances, 1);
    
    v = 9;
    assert_equal(static_cast<int>(v), 9);
    assert_equal(instances, 0);
}

TXL_UNIT_TEST(storage_union_get)
{
    auto v = txl::storage_union<int, double, std::string>{};
    v = 1234;
    assert_equal(v.get<int>(), 1234);
    
    v.set(43.21);
    assert_equal(v.get<double>(), 43.21);
}

TXL_UNIT_TEST(storage_union_has)
{
    auto v = txl::storage_union<int, double, std::string>{};
    assert_false(v.has<int>());
    assert_false(v.has<double>());
    assert_false(v.has<std::string>());
    
    v = 42;
    assert_true(v.has<int>());
    assert_false(v.has<double>());
    assert_false(v.has<std::string>());

    v = "hi";
    assert_false(v.has<int>());
    assert_false(v.has<double>());
    assert_true(v.has<std::string>());
    
    v.reset();
    assert_false(v.has<int>());
    assert_false(v.has<double>());
    assert_false(v.has<std::string>());
}

TXL_RUN_TESTS()
