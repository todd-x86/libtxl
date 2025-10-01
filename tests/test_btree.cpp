#include <txl/unit_test.h>
#include <txl/btree.h>

#include <string>

TXL_UNIT_TEST(simple)
{
    /*txl::btree<std::string, int> bt{3};
    bt.insert("Ted Crilly", 1);
    bt.insert("Dougal McGuire", 2);
    bt.insert("Mrs. Doyle", 3);
    bt.insert("Jack Hackett", 4);
    bt.insert("Noel Furlong", 5);
    bt.insert("Billy O'Dwyer", 6);
    bt.insert("Len Brennan", 7);
    bt.insert("Dick Byrne", 8);
    bt.insert("Cyril MacDuff", 9);*/
    
    txl::btree<int, int> bt{12};
    
    for (auto i = 100; i >= 0; i -= 1) {
      bt.insert(std::move(i), i*10);
    }
    /*for (auto i = 99; i >= 0; i -= 3) {
      bt.insert(std::move(i), i*10);
    }
    for (auto i = 98; i >= 0; i -= 3) {
      bt.insert(std::move(i), i*10);
    }*/
}


TXL_RUN_TESTS()
