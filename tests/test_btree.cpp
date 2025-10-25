#include <txl/unit_test.h>
#include <txl/btree.h>

#include <string>

TXL_UNIT_TEST(simple)
{
    /*
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

    bt.print();
    bt.remove("Mrs. Doyle");
    bt.print();
    bt.remove("Ted Crilly");
    bt.print();
    */
    txl::btree<int, int> bt{3};
    
    /*for (auto i = 20; i >= 0; i -= 1) {
      bt.insert(std::move(i), i*10);
    }

    bt.print();
    bt.remove(10);
    //bt.print();
    bt.remove(5);
    bt.remove(17);
    bt.remove(11);
    bt.remove(9);
    bt.remove(6);
    bt.print();*/

    std::vector<int> keys{};
    for (auto i = 1000; i > 0; --i)
    {
        bt.insert(std::move(i), i*10);
        keys.emplace_back(i);
    }
    std::random_shuffle(keys.begin(), keys.end());
    for (auto k : keys)
    {
        bt.remove(k);
    }
    bt.print();

    /*bt.insert(1,1);
    bt.insert(2,2);
    bt.insert(3,3);
    bt.insert(4,4);
    bt.insert(5,5);
    bt.insert(8,8);
    bt.insert(10,10);
    bt.insert(12,12);
    bt.insert(14,14);
    bt.insert(15,15);
    bt.insert(17,17);
    bt.insert(19,19);

    bt.print();
    //bt.remove(12);
    bt.remove(8);
    //bt.remove(17);
    bt.print();*/
}


TXL_RUN_TESTS()
