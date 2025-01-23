#include <txl/unit_test.h>
#include <txl/ring_buffer_file.h>
#include <txl/types.h>

#include <string_view>

using namespace std::literals;

TXL_UNIT_TEST(rb_file_write)
{
    auto f = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_write, (8 + 5) * 3};
    f.write("HELLO"sv).or_throw();
    f.write("HELLO"sv).or_throw();
    f.write("HELLO"sv).or_throw();
    f.write("DI-DA-DA-DA"sv).or_throw();

    auto f2 = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_only, (8 + 5) * 3};
    auto data = txl::byte_vector{};
    assert_equal("DI-DA-DA-DA"sv.size(), f2.read_into(data).or_throw().size());
    assert_equal(0, f2.read_into(data).or_throw().size());
    assert_equal(0, f2.read_into(data).or_throw().size());
    assert_equal(0, f2.read_into(data).or_throw().size());

    f.write("I CAN FIX IT!"sv).or_throw();
    data.clear();
    assert_equal("I CAN FIX IT!"sv.size(), f2.read_into(data).or_throw().size());
    assert_equal(0, f2.read_into(data).or_throw().size());
    assert_equal(0, f2.read_into(data).or_throw().size());
}

TXL_RUN_TESTS()
