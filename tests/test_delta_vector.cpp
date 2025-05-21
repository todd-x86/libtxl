#include <txl/unit_test.h>
#include <txl/delta_vector.h>
#include <txl/patterns.h>

TXL_UNIT_TEST(delta_vector_small)
{
    constexpr int32_t base = 10000;
    std::vector<int32_t> expected{base};
    txl::delta_vector<int32_t, int8_t> points{base};
    for (auto i = 1; i < 500; i += 15)
    {
        points.push_back(base+i);
        expected.push_back(base+i);
    }

    assert_equal(expected, txl::to_vector(points));
}

TXL_UNIT_TEST(delta_vector_value)
{
    constexpr int32_t base = 100000000;
    txl::delta_vector<int32_t, int8_t> points{base};
    std::vector<int32_t> expected{base};
    
    std::vector<int> deltas{100, 100, 100, -25, -25, 100, -25, 100, -25, 100, -25, 100};
    for (auto delta : deltas)
    {
        expected.push_back(base+delta);
        points.push_back(base+delta);
    }
    assert_equal(expected, txl::to_vector(points));
}

TXL_RUN_TESTS()
