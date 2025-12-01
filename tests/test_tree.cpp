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
    
    assert_equal(bst.find("Mrs. Doyle"), nullptr);
    assert_not_equal(bst.find("Ted Crilly"), nullptr);
}

TXL_UNIT_TEST(edge_case_1)
{
    /*
     *        (1)
     *           \
     *           (2)
     *              \
     *              (3)
     */
    txl::binary_search_tree<int, std::string> bst{};
    bst.emplace(1, "A");
    bst.emplace(2, "B");
    bst.emplace(3, "C");

    bst.remove(3);
    bst.remove(1);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(2)->second, "B");
    assert_equal(bst.find(3), nullptr);
}

TXL_UNIT_TEST(edge_case_2)
{
    /*
     *             (3)
     *             /
     *           (2)
     *          /
     *        (1)
     */
    txl::binary_search_tree<int, std::string> bst{};
    bst.emplace(3, "C");
    bst.emplace(2, "B");
    bst.emplace(1, "A");

    bst.remove(2);
    bst.remove(3);
    
    assert_equal(bst.find(1)->second, "A");
    assert_equal(bst.find(2), nullptr);
    assert_equal(bst.find(3), nullptr);
}

TXL_UNIT_TEST(edge_case_3)
{
    /*
     *             (2)
     *             / \
     *           (1) (3)
     */
    txl::binary_search_tree<int, std::string> bst{};
    bst.emplace(2, "B");
    bst.emplace(3, "C");
    bst.emplace(1, "A");

    bst.remove(2);
    bst.remove(1);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(2), nullptr);
    assert_equal(bst.find(3)->second, "C");
}

TXL_UNIT_TEST(edge_case_4)
{
    /*
     *              (10)
     *              /  \
     *           (5)   (15)
     *           / \
     *        (1)  (6)
     *          \
     *          (3)
     */
    txl::binary_search_tree<int, std::string> bst{};
    bst.emplace(10, "A");
    bst.emplace(15, "B");
    bst.emplace(5, "C");
    bst.emplace(1, "D");
    bst.emplace(3, "E");
    bst.emplace(6, "F");

    bst.remove(1);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(3)->second, "E");
    assert_equal(bst.find(5)->second, "C");
    assert_equal(bst.find(6)->second, "F");
    assert_equal(bst.find(10)->second, "A");
    assert_equal(bst.find(15)->second, "B");
    
    bst.remove(5);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(3)->second, "E");
    assert_equal(bst.find(5), nullptr);
    assert_equal(bst.find(6)->second, "F");
    assert_equal(bst.find(10)->second, "A");
    assert_equal(bst.find(15)->second, "B");
    
    bst.remove(10);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(3)->second, "E");
    assert_equal(bst.find(5), nullptr);
    assert_equal(bst.find(6)->second, "F");
    assert_equal(bst.find(10), nullptr);
    assert_equal(bst.find(15)->second, "B");
    
    bst.remove(15);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(3)->second, "E");
    assert_equal(bst.find(5), nullptr);
    assert_equal(bst.find(6)->second, "F");
    assert_equal(bst.find(10), nullptr);
    assert_equal(bst.find(15), nullptr);
    
    bst.remove(3);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(3), nullptr);
    assert_equal(bst.find(5), nullptr);
    assert_equal(bst.find(6)->second, "F");
    assert_equal(bst.find(10), nullptr);
    assert_equal(bst.find(15), nullptr);
    
    bst.remove(6);
    
    assert_equal(bst.find(1), nullptr);
    assert_equal(bst.find(3), nullptr);
    assert_equal(bst.find(5), nullptr);
    assert_equal(bst.find(6), nullptr);
    assert_equal(bst.find(10), nullptr);
    assert_equal(bst.find(15), nullptr);
}

TXL_UNIT_TEST(edge_case_5)
{
    /*
     *        (1)
     *           \
     *           (2)
     *              \
     *              (4)
     *             /   \
     *           (3)   (5)
     */
    txl::binary_search_tree<int, std::string> bst{};
    bst.emplace(1, "A");
    bst.emplace(2, "B");
    bst.emplace(4, "D");
    bst.emplace(3, "C");
    bst.emplace(5, "E");

    bst.remove(2);
    
    assert_equal(bst.find(1)->second, "A");
    assert_equal(bst.find(2), nullptr);
    assert_equal(bst.find(3)->second, "C");
    assert_equal(bst.find(4)->second, "D");
    assert_equal(bst.find(5)->second, "E");
    
    bst.remove(4);
    
    assert_equal(bst.find(1)->second, "A");
    assert_equal(bst.find(2), nullptr);
    assert_equal(bst.find(3)->second, "C");
    assert_equal(bst.find(4), nullptr);
    assert_equal(bst.find(5)->second, "E");
}


TXL_RUN_TESTS()
