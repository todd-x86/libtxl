#include <txl/unit_test.h>
#include <txl/buffer_ref.h>
#include <txl/types.h>

TXL_UNIT_TEST(buffer_ref)
{
    txl::byte_vector str_data{'H','e','l','l','o',' ','W','o','r','l','d'};
    auto ref = txl::buffer_ref{str_data.data(), str_data.size()};
    auto str_data_end = reinterpret_cast<void *>(std::next(reinterpret_cast<char *>(str_data.data()), str_data.size()));

    
    assert_equal(ref.size(), str_data.size());
    assert_equal(ref.begin(), str_data.data());
    assert_equal(ref.end(), str_data_end);

    auto ref_2 = ref.slice(1);
    assert_equal(ref_2.size(), str_data.size() - 1);
    assert_equal(ref_2.end(), str_data_end);

    auto ref_3 = ref.slice(0, 0);
    assert_equal(ref_3.size(), 0);

    auto ref_4 = ref.slice(0, 1);
    assert_equal(ref_4.size(), 1);
    assert_equal(ref_4.begin(), str_data.data());
    
    auto ref_5 = ref.slice(0, str_data.size() + 1);
    assert_equal(ref_5.size(), str_data.size());
    assert_equal(ref_5.end(), str_data_end);

    auto ref_6 = ref.slice(str_data.size());
    assert_equal(ref_6.size(), 0);

    auto ref_7 = ref.slice(str_data.size() - 2, str_data.size());
    auto ref_8 = ref.slice(str_data.size() - 2);
    assert_equal(ref_7.size(), 2);
    assert_equal(ref_7, ref_8);
}

TXL_RUN_TESTS()
