#include <txl/unit_test.h>
#include <txl/time.h>
#include <chrono>

TXL_UNIT_TEST(to_timespec)
{
    auto mono_now_tp = std::chrono::steady_clock::now();
    auto sys_now_tp = std::chrono::system_clock::now();

    auto mono_now_ts = txl::time::to_timespec(mono_now_tp);
    auto sys_now_ts = txl::time::to_timespec(sys_now_tp);

    auto new_mono_tp = txl::time::to_time_point<std::chrono::steady_clock::time_point>(mono_now_ts);
    auto new_sys_tp = txl::time::to_time_point<std::chrono::system_clock::time_point>(sys_now_ts);

    assert_equal(mono_now_tp, new_mono_tp);
    assert_equal(sys_now_tp, new_sys_tp);
}

TXL_RUN_TESTS()
