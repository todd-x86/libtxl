#include <txl/unit_test.h>
#include <txl/ring_buffer_file.h>

#include <string_view>

using namespace std::literals;

TXL_UNIT_TEST(rb_file_write)
{
    auto f = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_write, 4096};
    f.write("HELLO"sv).or_throw();
    f.write("HELLO"sv).or_throw();
    f.write("HELLO"sv).or_throw();
    f.write("DI-DA-DA-DA"sv).or_throw();

    auto f2 = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_only, 4096};
    assert_equal(5, f2.read().or_throw().size());
    assert_equal(5, f2.read().or_throw().size());
    assert_equal(5, f2.read().or_throw().size());
    auto data = f2.read().or_throw();
    assert_equal("DI-DA-DA-DA"sv.size(), data.size());
    assert_equal(0, f2.read().or_throw().size());
    assert_equal(0, f2.read().or_throw().size());
    assert_equal(0, f2.read().or_throw().size());

    assert_equal(9+9+9+15, f.offset());
    f.write("I CAN FIX IT!"sv).or_throw();
    assert_equal("I CAN FIX IT!"sv.size(), f2.read().or_throw().size());
    assert_equal(0, f2.read().or_throw().size());
    assert_equal(0, f2.read().or_throw().size());
}

TXL_UNIT_TEST(rb_file_write_circle)
{
    std::array<std::string_view, 5> rando{ "**"sv, "+++"sv, "-"sv, ",,,,"sv, "?"sv };
    std::ostringstream ss;
    auto f = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_write, 4096};
    for (auto i = 0; i < 1000; ++i)
    {
        ss.str("");
        ss << "HELLO" << i << rando[i % rando.size()];
        f.write(ss.str()).or_throw();
    }

    auto f2 = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_only, 4096};
    for (auto i = 0; i < 50; ++i)
    {
        auto j = i + 911;
        ss.str("");
        ss << "HELLO" << j << rando[j % rando.size()];
        auto s = f2.read().or_throw();
        assert_equal(s.to_string_view(), ss.str());
    }
}

TXL_UNIT_TEST(rb_file_write_circle_stale_reader)
{
    std::array<std::string_view, 5> rando{ "**"sv, "+++"sv, "-"sv, ",,,,"sv, "?"sv };
    std::ostringstream ss;
    auto f = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_write, 4096};
    for (auto i = 0; i < 500; ++i)
    {
        ss.str("");
        ss << "HELLO" << i << rando[i % rando.size()];
        f.write(ss.str()).or_throw();
    }

    // Introduce a stale reader...
    auto f2 = txl::ring_buffer_file{"test.bin", txl::ring_buffer_file::read_only, 4096};

    // Write some more
    for (auto i = 500; i < 1000; ++i)
    {
        ss.str("");
        ss << "HELLO" << i << rando[i % rando.size()];
        f.write(ss.str()).or_throw();
    }

    for (auto i = 0; i < 50; ++i)
    {
        auto j = i + 911;
        ss.str("");
        ss << "HELLO" << j << rando[j % rando.size()];
        auto s = f2.read().or_throw();
        assert_equal(s.to_string_view(), ss.str());
    }
}

TXL_RUN_TESTS()
