#include <txl/unit_test.h>
#include <txl/iterator_view.h>

#include <vector>
#include <list>
#include <map>

TXL_UNIT_TEST(common_iterator_views)
{
    {
        std::vector<int> nums{1,2,3,4,5,6};
        int sum = 0;
        for (auto num : txl::make_iterator_view(nums.begin(), nums.end()))
        {
            sum += num;
        }
        assert(sum == 1+2+3+4+5+6);
    }
    {
        std::list<int> nums{1,2,3,4,5,6};
        int sum = 0;
        for (auto num : txl::make_iterator_view(nums.begin(), nums.end()))
        {
            sum += num;
        }
        assert(sum == 1+2+3+4+5+6);
    }
    {
        std::map<int, double> nums{{1,1.0},{2,3.0},{3,6.0},{4,10.0},{5,15.0},{6,21.0}};
        double sum = 0.0;
        for (auto const & item : txl::make_iterator_view(nums.begin(), nums.end()))
        {
            sum += (item.first * item.second);
        }
        assert(sum == (1*1.0)+(2*3.0)+(3*6.0)+(4*10.0)+(5*15.0)+(6*21.0));
    }
}

struct test_reader
{
    int curr_;

    auto next() { curr_++; }
};

class test_iterator
{
private:
    test_reader & reader_;
    int curr_;
    int index_;
    int limit_;
public:
    using difference_type = size_t;
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using pointer = int const *;
    using reference = int const &;

    test_iterator(test_reader & reader, int index, int limit)
        : reader_(reader)
        , curr_(reader.curr_)
        , index_(index)
        , limit_(limit)
    {
    }

    auto operator*() const -> int { return curr_; }
    auto operator->() const -> int const * { return &curr_; }
    auto operator++() -> test_iterator &
    {
        if (index_ < limit_)
        {
            reader_.next();
            ++index_;
            curr_ = reader_.curr_;
        }
        return *this;
    }

    auto operator==(test_iterator const & it) const -> bool
    {
        return limit_ == it.limit_
           and index_ == it.index_;
    }
    auto operator!=(test_iterator const & it) const -> bool
    {
        return !(*this == it);
    }
};

TXL_UNIT_TEST(custom_iterator)
{
    auto rd = test_reader{0};
    auto it = txl::make_iterator_view(test_iterator(rd, 0, 5), test_iterator(rd, 5, 5));
    assert(not it.empty());
    assert(std::distance(it.begin(), it.end()) == 5);
}

TXL_RUN_TESTS()
