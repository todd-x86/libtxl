#include <txl/unit_test.h>
#include <txl/find.h>

#include <string_view>

using namespace std::literals;

TXL_UNIT_TEST(stream_find)
{
    txl::stream_find find{"LALAND"sv};
    assert_false(find.is_matched());
    assert_equal(find.num_bytes_processed(), 0);
    find.process("LALALAND"sv);
    assert_true(find.is_matched());
    assert_equal(find.num_bytes_processed(), "LALALAND"sv.size());

    // Once we match, we stop processing
    find.process("LALALALALAND"sv);
    assert_true(find.is_matched());
    assert_equal(find.num_bytes_processed(), "LALALAND"sv.size());
    
    // Reset
    find.reset();
    assert_false(find.is_matched());
    assert_equal(find.num_bytes_processed(), 0);
    
    // Process again
    find.process("BLABLABLA"sv);
    assert_false(find.is_matched());
    assert_equal(find.num_bytes_processed(), "BLABLABLA"sv.size());
    
    // Process slowly
    find.process("LA"sv);
    assert_false(find.is_matched());
    assert_equal(find.num_bytes_processed(), "BLABLABLALA"sv.size());
    find.process("N"sv);
    assert_false(find.is_matched());
    assert_equal(find.num_bytes_processed(), "BLABLABLALAN"sv.size());
    find.process("DOVER"sv);    // note: stopped before "OVER"
    assert_true(find.is_matched());
    assert_equal(find.num_bytes_processed(), "BLABLABLALAND"sv.size());
}

TXL_RUN_TESTS()
