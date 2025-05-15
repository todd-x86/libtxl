#include <txl/unit_test.h>
#include <txl/delta_vector.h>
#include <txl/patterns.h>

TXL_UNIT_TEST(delta_vector_value)
{
    constexpr int32_t base = 100000000;
    txl::delta_vector<int32_t, int8_t> points{base};
    points.push_back(base+100);
    points.push_back(base+100);
    points.push_back(base+100);
    points.push_back(base-25);
    points.push_back(base-25);
    points.push_back(base+100);
    points.push_back(base-25);
    points.push_back(base+100);
    points.push_back(base-125);
    points.push_back(base+100);
    points.push_back(base-125);
    points.push_back(base+100);
    points.push_back(base-125);
    points.push_back(base+100);
    points.push_back(base-125);
    points.push_back(base+100);

    std::vector<int32_t> expected{base, base+100, base+100, base+100, base-25, base-25, base+100, base-25, base+100, base-125, base+100, base-125, base+100, base-125, base+100, base-125, base+100};
    assert_equal(expected, txl::to_vector(points));
}

TXL_UNIT_TEST(delta_vector_delta)
{
    constexpr int32_t base = 100000000;
    txl::delta_vector<int32_t, int8_t> points{base};
    points.push_back_delta(100);
    points.push_back_delta(100);
    points.push_back_delta(100);
    points.push_back_delta(-25);
    points.push_back_delta(-25);
    points.push_back_delta(100);
    points.push_back_delta(-25);
    points.push_back_delta(100);
    points.push_back_delta(-125);
    points.push_back_delta(100);
    points.push_back_delta(-125);
    points.push_back_delta(100);
    points.push_back_delta(-125);
    points.push_back_delta(100);
    points.push_back_delta(-125);
    points.push_back_delta(100);

    std::vector<int32_t> expected{base, base+100, base+100, base+100, base-25, base-25, base+100, base-25, base+100, base-125, base+100, base-125, base+100, base-125, base+100, base-125, base+100};
    assert_equal(expected, txl::to_vector(points));
}

TXL_RUN_TESTS()
