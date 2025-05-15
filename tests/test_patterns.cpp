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

    assert(txl::if_found(names, 10, [this](auto it) {
        assert(it->first == 10);
        assert(it->second == "Tad Ghostal");
    }));
    
    assert(not txl::if_found(names, 30, [this](auto it) {
        assert(it->first == 30);
        assert(it->second == "Zorak");
        assert(false);
    }));

    std::vector<int> nums{9,8,7,6,5};
    
    assert(txl::if_found(nums, 5, [this](auto it) {
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

auto copy_all(txl::virtual_iterator_view<std::string> const & src, std::vector<std::string> & dst)
{
    for (auto const & el : src)
    {
        dst.emplace_back(el);
    }
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
    
    for (auto i = 0; i < 1000; ++i)
    {
        assert_equal(src[i], dst[i]);
    }
}

TXL_UNIT_TEST(test_virtual_iterator)
{
    std::vector<std::string> src{}, dst{};

    std::ostringstream ss{};

    for (auto i = 0; i < 1000; ++i)
    {
        ss.str("");
        ss << "This is a string " << i;
        src.emplace_back(ss.str());
    }

    auto begin = txl::virtual_iterator_wrapper{src.begin()};
    auto end = txl::virtual_iterator_wrapper{src.end()};
    copy_all({begin, end}, dst);

    assert_equal(dst.size(), src.size());
    
    for (auto i = 0; i < 1000; ++i)
    {
        assert_equal(src[i], dst[i]);
    }
}

TXL_UNIT_TEST(test_to_vector)
{
    std::map<std::string, int> inventory{{"Health", 90}, {"Armor", 25}, {"Revolver", 15}, {"Rocket Launcher", 4}};

    std::vector<std::pair<std::string const, int>> expected{{"Armor", 25}, {"Health", 90}, {"Revolver", 15}, {"Rocket Launcher", 4}};
    assert_equal(expected, txl::to_vector(inventory));
}

TXL_UNIT_TEST(test_to_map)
{
    std::map<std::string, int> inventory{{"Health", 90}, {"Armor", 25}, {"Revolver", 15}, {"Rocket Launcher", 4}};

    assert_equal(inventory, txl::to_map(txl::to_vector(inventory)));
}

TXL_UNIT_TEST(test_to_unordered_map)
{
    std::unordered_map<std::string, int> inventory{{"Health", 90}, {"Armor", 25}, {"Revolver", 15}, {"Rocket Launcher", 4}};

    assert_equal(inventory, txl::to_unordered_map(txl::to_vector(inventory)));
}

TXL_RUN_TESTS()
