#include <txl/unit_test.h>
#include <txl/btree.h>

#include <string>

TXL_UNIT_TEST(simple)
{
    txl::btree<std::string, int> bt{3};
    bt.insert("Ted Crilly", 1);
    bt.insert("Dougal McGuire", 2);
    bt.insert("Mrs. Doyle", 3);
    bt.insert("Jack Hackett", 4);
    bt.insert("Noel Furlong", 5);
    bt.insert("Billy O'Dwyer", 6);
    bt.insert("Len Brennan", 7);
    bt.insert("Dick Byrne", 8);
    bt.insert("Cyril MacDuff", 9);
}


TXL_RUN_TESTS()
