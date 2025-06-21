#include <txl/unit_test.h>
#include <txl/fixed_vector.h>

#include <string>
#include <vector>

TXL_UNIT_TEST(fixed_vector)
{
    txl::fixed_vector<std::string, 3> names{};
    std::vector<std::string> out{};

    assert_equal(names.max_size(), 3);
    assert_equal(names.capacity(), 3);
    assert_equal(out.size(), 0);
    assert_equal(names.size(), out.size());
    for (auto const & name : names)
    {
        out.emplace_back(name);
    }
}

TXL_UNIT_TEST(push_back)
{
    txl::fixed_vector<std::string, 3> names{};
    assert_equal(names.size(), 0);
    names.push_back("Rocko");
    assert_equal(names.size(), 1);
    assert_equal(names[0], "Rocko");

    auto x = std::move(names[0]);
    names.pop_back();
    assert_equal(names.size(), 0);
    assert_equal(x, "Rocko");
}

struct grocery final
{
    std::string name;
    int quantity;
    double price;

    grocery(std::string && name, int quantity, double price)
        : name{std::move(name)}
        , quantity{quantity}
        , price{price}
    {
    }
};

TXL_UNIT_TEST(emplace_back)
{
    txl::fixed_vector<grocery, 8> groceries{};
    groceries.emplace_back("Bread", 1, 2.99);
    groceries.emplace_back("Milk", 2, 1.99);
    groceries.emplace_back("Tomatoes", 8, 1.99);

    assert_equal(groceries.size(), 3);
}

TXL_UNIT_TEST(insert_middle)
{
    txl::fixed_vector<grocery, 8> groceries{};
    groceries.emplace_back("Bread", 1, 2.99);
    groceries.emplace_back("Tomatoes", 8, 1.99);
    groceries.insert(groceries.begin() + 1, grocery{"Milk", 2, 1.99});

    assert_equal(groceries.size(), 3);
    assert_equal(groceries[0].name, "Bread");
    assert_equal(groceries[1].name, "Milk");
    assert_equal(groceries[2].name, "Tomatoes");
}

TXL_RUN_TESTS()
