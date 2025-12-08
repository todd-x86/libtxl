#include <txl/sorted.h>
#include <txl/unit_test.h>

#include <list>
#include <vector>

TXL_UNIT_TEST(sorted_list_1)
{
    txl::sorted<std::list<std::string>> x{};
    x.emplace_sorted("HELLO");
    x.emplace_sorted("AND");
    x.emplace_sorted("GOODBYE");
    x.emplace_sorted("TO");
    x.emplace_sorted("EVERYONE");
    x.emplace_sorted("ONE");
    x.emplace_sorted("AND");
    x.emplace_sorted("ALL");

    assert_equal(x, std::list<std::string>{"ALL","AND","AND","EVERYONE","GOODBYE","HELLO","ONE","TO"});
}

TXL_UNIT_TEST(sorted_vector_1)
{
    txl::sorted<std::vector<std::string>> x{};
    x.emplace_sorted("HELLO");
    x.emplace_sorted("AND");
    x.emplace_sorted("GOODBYE");
    x.emplace_sorted("TO");
    x.emplace_sorted("EVERYONE");
    x.emplace_sorted("ONE");
    x.emplace_sorted("AND");
    x.emplace_sorted("ALL");
    
    assert_equal(x, std::vector<std::string>{"ALL","AND","AND","EVERYONE","GOODBYE","HELLO","ONE","TO"});
}

TXL_UNIT_TEST(sorted_list_2)
{
    txl::sorted<std::list<std::string>> x{"HELLO","AND","GOODBYE","TO","EVERYONE","ONE","AND","ALL"};

    assert_equal(x, std::list<std::string>{"ALL","AND","AND","EVERYONE","GOODBYE","HELLO","ONE","TO"});
}

TXL_UNIT_TEST(sorted_vector_2)
{
    txl::sorted<std::vector<std::string>> x{"HELLO","AND","GOODBYE","TO","EVERYONE","ONE","AND","ALL"};
    
    assert_equal(x, std::vector<std::string>{"ALL","AND","AND","EVERYONE","GOODBYE","HELLO","ONE","TO"});
}

TXL_RUN_TESTS()
