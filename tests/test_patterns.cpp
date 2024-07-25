#include <txl/unit_test.h>
#include <txl/patterns.h>

#include <map>
#include <unordered_map>

TXL_UNIT_TEST(test_find_or_emplace_map)
{
    std::map<int, std::string> data{};

    auto it = txl::find_or_emplace(data, 123, [](auto &) { return "bar"; });
    assert(it->first == 123);
    assert(it->second == "bar");
    
    it = txl::find_or_emplace(data, 123, [](auto &) { return "foo"; });
    assert(it->first == 123);
    assert(it->second == "bar");
}

TXL_UNIT_TEST(test_find_or_emplace_unordered_map)
{
    std::unordered_map<int, std::string> data{};

    auto it = txl::find_or_emplace(data, 123, [](auto &) { return "bar"; });
    assert(it->first == 123);
    assert(it->second == "bar");
    
    it = txl::find_or_emplace(data, 123, [](auto &) { return "foo"; });
    assert(it->first == 123);
    assert(it->second == "bar");
}

TXL_UNIT_TEST(test_find_or_emplace_value)
{
    std::map<int, std::string> data{};

    auto it = txl::find_or_emplace(data, 123, "fizzbuzz");
    assert(it->first == 123);
    assert(it->second == "fizzbuzz");
}

TXL_RUN_TESTS()
