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

TXL_RUN_TESTS()
