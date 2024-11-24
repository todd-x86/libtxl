#include <txl/unit_test.h>
#include <txl/copy.h>
#include <txl/file.h>
#include <txl/read_string.h>
#include <txl/stream_writer.h>
#include <txl/types.h>

#include <sstream>
#include <array>

TXL_UNIT_TEST(file_write_read)
{
    {
        auto f = txl::file{"sample.txt", "w"};
        auto data = std::string_view{"GUTEN ABEND"};
        auto written = f.write(txl::buffer_ref{data}).or_throw();
        assert_equal(written.size(), 11);
    }

    {
        auto f = txl::file{"sample.txt", "r"};
        std::array<char, 32> buf{0};
        assert_equal(f.tell().or_throw(), 0);
        assert_equal(f.seekable_size().or_throw(), 11);
        assert_equal(f.tell().or_throw(), 0);
        auto input = f.read(txl::buffer_ref{reinterpret_cast<void *>(&buf), buf.size()}).or_throw();
        assert_equal(input.size(), 11);
        assert_equal(std::string_view{&buf[0]}, std::string_view{"GUTEN ABEND"});
    }
}

TXL_UNIT_TEST(file_append)
{
    // Initial write
    {
        auto f = txl::file{"sample3.txt", "w"};
        auto written = f.write(std::string_view{"This is a test"}).or_throw();
        assert_equal(written.size(), std::string_view{"This is a test"}.length());
    }

    // Append
    {
        auto f = txl::file{"sample3.txt", "a"};
        auto written = f.write(std::string_view{" for you"}).or_throw();
        assert_equal(written.size(), std::string_view{" for you"}.length());
    }

    // Read it all
    {
        auto f = txl::file{"sample3.txt", "a+"};
        assert_equal(f.tell().or_throw(), 0);

        auto dst = std::stringstream{};
        auto dst_adapter = txl::stream_writer{dst};
        auto buf = txl::byte_array<512>{};
        auto bytes_read = txl::copy(f, dst_adapter, buf).or_throw();
        assert_equal(bytes_read, std::string_view{"This is a test for you"}.length());
        assert_equal(dst.str(), "This is a test for you");
    }
}

TXL_UNIT_TEST(file_read_buffer)
{
    // Initial write
    {
        auto f = txl::file{"sample3.txt", "w"};
        auto written = f.write(std::string_view{"This is a test"}).or_throw();
        assert_equal(written.size(), std::string_view{"This is a test"}.length());
    }

    // Read it through a buffer, one byte at a time
    {
        auto f = txl::file{"sample3.txt", "a+"};

        auto dst = std::stringstream{};
        auto dst_adapter = txl::stream_writer{dst};
        auto buf = txl::byte_array<1>{};
        while (true)
        {
            auto bytes_read = txl::copy(f, dst_adapter, buf).or_throw();
            if (bytes_read == 0)
            {
                break;
            }
        }
        assert_equal(dst.str(), "This is a test");
    }
}

TXL_UNIT_TEST(file_modes)
{
    assert_throws<std::system_error>([]() {
        auto f = txl::file{"not-a-file.txt", "r"};
    });
}

TXL_UNIT_TEST(file_seek)
{
    auto f = txl::file{"data.txt", "w+"};
    assert_true(f.is_open());

    auto s = f.write(std::string_view{"HAHAHA! I wet 'em."}).or_throw();
    assert_equal(s, std::string_view{"HAHAHA! I wet 'em."});

    // Seek from end
    f.seek(-10, txl::file::seek_end).or_throw();
    auto pepperpot = txl::read_string(f, 10).or_throw();
    assert_equal(std::string_view{pepperpot}, std::string_view{"I wet 'em."});
    
    // Seek backwards
    f.seek(-10, txl::file::seek_current).or_throw();
    f.seek(-4, txl::file::seek_current).or_throw();
    auto guffaw = txl::read_string(f, 2).or_throw();
    assert_equal(std::string_view{guffaw}, std::string_view{"HA"});
    
    // Seek set
    f.seek(3, txl::file::seek_set).or_throw();
    auto laugh = txl::read_string(f, 4).or_throw();
    assert_equal(std::string_view{laugh}, std::string_view{"AHA!"});
}

TXL_RUN_TESTS()
