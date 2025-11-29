#include <algorithm>
#include <random>
#include <txl/unit_test.h>
#include <txl/tree.h>

#include <string>

TXL_UNIT_TEST(simple)
{
    txl::binary_search_tree<std::string, int> bst{};
    bst.emplace("Ted Crilly", 1);
    bst.emplace("Dougal McGuire", 2);
    bst.emplace("Mrs. Doyle", 3);
    bst.emplace("Jack Hackett", 4);
    bst.emplace("Noel Furlong", 5);
    bst.emplace("Billy O'Dwyer", 6);
    bst.emplace("Len Brennan", 7);
    bst.emplace("Dick Byrne", 8);
    bst.emplace("Cyril MacDuff", 9);
    bst.emplace("Ted Crilly", 2);
    bst.emplace("Ted Crilly", 3);

    assert_equal(bst.find("Len Brennan")->second, 7);
    assert_equal(bst.find("Tom"), nullptr);


    bst.remove("Mrs. Doyle");
    bst.remove("Ted Crilly");
    bst.print();
    assert_equal(bst.find("Mrs. Doyle"), nullptr);
    assert_not_equal(bst.find("Ted Crilly"), nullptr);
}


TXL_RUN_TESTS()
