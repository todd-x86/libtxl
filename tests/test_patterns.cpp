#include <txl/unit_test.h>
#include <txl/patterns.h>

#include <map>
#include <unordered_map>
#include <vector>

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

TXL_UNIT_TEST(test_if_found)
{
    std::map<int, std::string> names{ {10, "Tad Ghostal"}, {20, "Brak"} };

    assert(txl::if_found(names, 10, [](auto it) {
        assert(it->first == 10);
        assert(it->second == "Tad Ghostal");
    }));
    
    assert(not txl::if_found(names, 30, [](auto it) {
        assert(it->first == 30);
        assert(it->second == "Zorak");
        assert(false);
    }));

    std::vector<int> nums{9,8,7,6,5};
    
    assert(txl::if_found(nums, 5, [](auto it) {
        assert(*it == 5);
    }));
}

TXL_UNIT_TEST(test_try_emplace)
{
    std::unordered_map<int, std::string> data{};

    auto emplaced = txl::try_emplace(data, 123, [](auto &) { return "bar"; });
    assert(emplaced);
    emplaced = txl::try_emplace(data, 123, [](auto &) { return "blablabla"; });
    assert(not emplaced);
}

TXL_RUN_TESTS()
