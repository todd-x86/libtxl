#include <txl/unit_test.h>
#include <txl/file.h>
#include <txl/on_error.h>

#include <sstream>
#include <array>

TXL_UNIT_TEST(file_write_read)
{
    {
        auto f = txl::file{"sample.txt", "w"};
        auto data = std::string_view{"GUTEN ABEND"};
        auto written = f.write(txl::buffer_ref{data});
        assert_equal(written.size(), 11);
    }

    {
        auto f = txl::file{"sample.txt", "r"};
        std::array<char, 32> buf{0};
        auto input = f.read(txl::buffer_ref{reinterpret_cast<void *>(&buf), buf.size()});
        assert_equal(input.size(), 11);
        assert_equal(std::string_view{&buf[0]}, std::string_view{"GUTEN ABEND"});
    }
}

TXL_UNIT_TEST(file_append)
{
    // Initial write
    {
        auto f = txl::file{"sample3.txt", "w"};
        auto written = f.write(std::string_view{"This is a test"});
        assert_equal(written.size(), std::string_view{"This is a test"}.length());
    }

    // Append
    {
        auto f = txl::file{"sample3.txt", "a"};
        auto written = f.write(std::string_view{" for you"});
        assert_equal(written.size(), std::string_view{" for you"}.length());
    }

    // Read it all
    {
        auto f = txl::file{"sample3.txt", "a+"};

        std::stringstream buf{};
        auto bytes_read = f.read(buf, 512);
        assert_equal(bytes_read, std::string_view{"This is a test for you"}.length());
        assert_equal(buf.str(), "This is a test for you");
    }
}

TXL_UNIT_TEST(file_read_buffer)
{
    // Initial write
    {
        auto f = txl::file{"sample3.txt", "w"};
        auto written = f.write(std::string_view{"This is a test"});
        assert_equal(written.size(), std::string_view{"This is a test"}.length());
    }

    // Read it through a buffer, one byte at a time
    {
        auto f = txl::file{"sample3.txt", "a+"};

        std::stringstream buf{};
        while (true)
        {
            auto bytes_read = f.read(buf, 1);
            if (bytes_read == 0)
            {
                break;
            }
        }
        assert_equal(buf.str(), "This is a test");
    }
}

TXL_UNIT_TEST(file_modes)
{
    auto f = txl::file{"not-a-file.txt", "r", txl::on_error::ignore{}};
    assert_false(f.is_open());
}

TXL_UNIT_TEST(file_error_capture)
{
    txl::system_error err{};
    assert_false(err.is_error());

    auto f = txl::file{"not-a-file.txt", "r", txl::on_error::capture(err)};
    assert_false(f.is_open());
    assert_true(err.is_error());
}

TXL_RUN_TESTS()
