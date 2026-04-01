#include <txl/unit_test.h>
#include <txl/csv.h>
#include <txl/io_buffer.h>

#include <string_view>
#include <vector>

TXL_UNIT_TEST(csv_splitter)
{
    txl::csv::string_view_splitter<> splitter{"Hello,World,123", ','};
    std::ostringstream ss{};

    assert_equal(splitter.next(ss), txl::csv::split_status::delimiter);
    assert_equal(ss.str(), "Hello");
    ss.str("");

    assert_equal(splitter.next(ss), txl::csv::split_status::delimiter);
    assert_equal(ss.str(), "World");
    ss.str("");

    assert_equal(splitter.next(ss), txl::csv::split_status::end_of_line);
    assert_equal(ss.str(), "123");
    ss.str("");

    assert_equal(splitter.next(ss), txl::csv::split_status::empty);
    assert_equal(ss.str(), "");
}

TXL_UNIT_TEST(csv_parse_line)
{
    using namespace std::literals;
    txl::io_buffer buf{};
    buf.write("HELLO,123,BLA BLA\n"sv);
    buf.write("Goodbye,987,\"WOMP WOMP\",\"QUOTE\"\"QUOTE\"\n"sv);

    std::vector<std::string> row{};
    txl::csv::parse_line(buf, [&](std::string && col) {
        row.emplace_back(std::move(col));
    });
    assert_equal(row, std::vector<std::string>{"HELLO", "123", "BLA BLA"});
    
    row.clear();
    txl::csv::parse_line(buf, [&](std::string && col) {
        row.emplace_back(std::move(col));
    });

    assert_equal(row, std::vector<std::string>{"Goodbye", "987", "WOMP WOMP", "QUOTE\"QUOTE"});
    
    row.clear();
    txl::csv::parse_line(buf, [&](std::string && col) {
        row.emplace_back(std::move(col));
    });
    assert_equal(row.size(), 0);
}

TXL_RUN_TESTS()
