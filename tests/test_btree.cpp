#include <algorithm>
#include <random>
#include <txl/unit_test.h>
#include <txl/btree.h>

#include <string>

TXL_UNIT_TEST(simple)
{
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

        bt.print();
        bt.remove("Mrs. Doyle");
        bt.print();
        bt.remove("Ted Crilly");
        bt.print();
    }

    {
        txl::btree<int, int> bt{3};
        
        std::vector<int> keys{};
        for (auto i = 1000; i > 0; --i)
        {
            bt.insert(std::move(i), i*10);
            keys.emplace_back(i);
        }
	std::random_device rd;
    // std::mt19937 is a Mersenne Twister engine, a good general-purpose URNG
    std::mt19937 g(rd());

    // Shuffle the vector using std::shuffle and the random engine
        std::shuffle(keys.begin(), keys.end(), g);
        for (auto k : keys)
        {
            if (k > 57 or k < 53)
            {
                bt.remove(k);
            }
        }
        bt.print();
    }
}


TXL_RUN_TESTS()
