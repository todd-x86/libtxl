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

TXL_UNIT_TEST(test_emplace_or_update_map)
{
    std::map<int, std::string> data{};

    auto it = txl::emplace_or_update(data, 123, "bar");
    assert(it->first == 123);
    assert(it->second == "bar");
    
    it = txl::emplace_or_update(data, 123, [](auto &) { return "foo"; });
    assert(it->first == 123);
    assert(it->second == "foo");
}

TXL_UNIT_TEST(test_emplace_or_update_unordered_map)
{
    std::unordered_map<int, std::string> data{};

    auto it = txl::emplace_or_update(data, 123, [](auto &) { return "bar"; });
    assert(it->first == 123);
    assert(it->second == "bar");
    
    it = txl::emplace_or_update(data, 123, "foo");
    assert(it->first == 123);
    assert(it->second == "foo");
}

auto copy_all(txl::foreach_view<std::string> const & src, std::vector<std::string> & dst)
{
    src.foreach([&](auto const & el) {
        dst.emplace_back(el);
    });
}

TXL_UNIT_TEST(test_foreach_view)
{
    std::vector<std::string> src{}, dst{};

    std::ostringstream ss{};

    for (auto i = 0; i < 1000; ++i)
    {
        ss.str("");
        ss << "This is a string " << i;
        src.emplace_back(ss.str());
    }

    copy_all(txl::container_foreach_view{src}, dst);

    assert_equal(dst.size(), src.size());
}


TXL_RUN_TESTS()
