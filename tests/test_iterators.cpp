#include <txl/unit_test.h>
#include <txl/iterators.h>

TXL_UNIT_TEST(circular_iterator)
{
    auto s = std::string_view{"1234567890"};
    auto it = txl::make_circular_iterator(s.begin(), s.end());

    assert_equal(*it, '1');
    ++it;
    assert_equal(*it, '2');
    it++;
    assert_equal(*it, '3');
    it += 2;
    assert_equal(*it, '5');
    it += 5;
    assert_equal(*it, '0');
    it += 1;
    assert_equal(*it, '1');

    it--;
    it++;
    it--;
    assert_equal(*it, '0');

    it += 12;
    assert_equal(*it, '2');
    it -= 18;
    assert_equal(*it, '4');
    
    it += 9;
    assert_equal(*it, '3');
}

TXL_RUN_TESTS()
