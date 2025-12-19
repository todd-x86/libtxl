#include <txl/set.h>
#include <txl/unit_test.h>
#include <txl/patterns.h>

#include <vector>
#include <string>

TXL_UNIT_TEST(set)
{
    txl::set<std::string> x{};
    x.add("Carrots");
    x.add("Broccoli");
    x.add("Asparagus");

    assert_equal(txl::to_vector(x), std::vector<std::string>{"Asparagus", "Broccoli", "Carrots"});
}

TXL_UNIT_TEST(set_intersection)
{
    txl::set<std::string> x{};
    x.add("Carrots");
    x.add("Broccoli");
    x.add("Asparagus");
    
    txl::set<std::string> y{};
    y.add("Cauliflower");
    y.add("Broccoli");
    y.add("Asparagus");

    auto z = x.intersect(y);

    assert_equal(txl::to_vector(z), std::vector<std::string>{"Asparagus", "Broccoli"});
}

TXL_UNIT_TEST(set_union)
{
    txl::set<std::string> x{};
    x.add("Carrots");
    x.add("Broccoli");
    x.add("Asparagus");
    
    txl::set<std::string> y{};
    y.add("Cauliflower");
    y.add("Broccoli");
    y.add("Asparagus");

    auto z = x.merge(y);

    assert_equal(txl::to_vector(z), std::vector<std::string>{"Asparagus", "Broccoli", "Carrots", "Cauliflower"});
}

TXL_UNIT_TEST(set_add)
{
    txl::set<std::string> x{};
    x.add("Carrots");
    x.add("Carrots");
    x.add("Broccoli");
    x.add("Broccoli");
    x.add("Asparagus");
    x.add("Asparagus");
    
    assert_equal(txl::to_vector(x), std::vector<std::string>{"Asparagus", "Broccoli", "Carrots"});
}

struct vegetable final
{
    std::string name;
    int quantity;

    auto operator<(vegetable const & v) const -> bool
    {
        return name.compare(v.name) < 0;
    }

    auto operator==(vegetable const & v) const -> bool
    {
        return name == v.name and quantity == v.quantity;
    }
};

inline auto operator<<(std::ostream & os, vegetable const & v) -> std::ostream &
{
    os << v.name << " (" << v.quantity << " quantity)";
    return os;
}

TXL_UNIT_TEST(set_intersection_custom)
{
    txl::set<vegetable> x{};
    x.add(vegetable{"Carrots", 1});
    x.add(vegetable{"Broccoli", 2});
    x.add(vegetable{"Asparagus", 3});
    
    txl::set<std::string> y{};
    y.add("Cauliflower");
    y.add("Broccoli");
    y.add("Asparagus");

    auto z = x.intersect(y, [](auto const & v) { return v.name; });

    assert_equal(txl::to_vector(z), std::vector<vegetable>{vegetable{"Asparagus", 3}, vegetable{"Broccoli", 2}});
}

TXL_RUN_TESTS()
