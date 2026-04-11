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

TXL_UNIT_TEST(csv_parse_document)
{
    using namespace std::literals;
    txl::io_buffer buf{};
    buf.write("ID,First Name,Last Name\n"sv);
    buf.write("1,Homer,Simpson\n"sv);
    buf.write("2,Marge,Simpson\n"sv);
    buf.write("3,Bart,Simpson\n"sv);
    buf.write("4,Lisa,Simpson\n"sv);
    buf.write("5,Maggie,Simpson"sv);

    txl::csv::document<> doc{};
    txl::csv::parse_document(buf, doc);

    assert_equal(doc.size(), 6);
    assert_equal(doc[0].size(), 3);
    assert_equal(doc[1].size(), 3);
    assert_equal(doc[2].size(), 3);
    assert_equal(doc[3].size(), 3);
    assert_equal(doc[4].size(), 3);
    assert_equal(doc[5].size(), 3);

    assert_equal(doc[0].data(), txl::csv::row<>{"ID", "First Name", "Last Name"});
    assert_equal(doc[1].data(), txl::csv::row<>{"1", "Homer", "Simpson"});
    assert_equal(doc[2].data(), txl::csv::row<>{"2", "Marge", "Simpson"});
    assert_equal(doc[3].data(), txl::csv::row<>{"3", "Bart", "Simpson"});
    assert_equal(doc[4].data(), txl::csv::row<>{"4", "Lisa", "Simpson"});
    assert_equal(doc[5].data(), txl::csv::row<>{"5", "Maggie", "Simpson"});
}

TXL_UNIT_TEST(csv_parse_header_document)
{
    using namespace std::literals;
    txl::io_buffer buf{};
    buf.write("ID,First Name,Last Name\n"sv);
    buf.write("1,Homer,Simpson\n"sv);
    buf.write("2,Marge,Simpson\n"sv);
    buf.write("3,Bart,Simpson\n"sv);
    buf.write("4,Lisa,Simpson\n"sv);
    buf.write("5,Maggie,Simpson"sv);

    txl::csv::document<> doc{true};
    txl::csv::parse_document(buf, doc);

    assert_equal(doc.size(), 5);
    assert_equal(doc[0].size(), 3);
    assert_equal(doc[1].size(), 3);
    assert_equal(doc[2].size(), 3);
    assert_equal(doc[3].size(), 3);
    assert_equal(doc[4].size(), 3);

    assert_equal(doc[0].data(), txl::csv::row<>{"1", "Homer", "Simpson"});
    assert_equal(doc[1].data(), txl::csv::row<>{"2", "Marge", "Simpson"});
    assert_equal(doc[2].data(), txl::csv::row<>{"3", "Bart", "Simpson"});
    assert_equal(doc[3].data(), txl::csv::row<>{"4", "Lisa", "Simpson"});
    assert_equal(doc[4].data(), txl::csv::row<>{"5", "Maggie", "Simpson"});
    
    assert_equal(doc[0]["First Name"], "Homer");
    assert_equal(doc[0]["Last Name"], "Simpson");
    assert_equal(doc[3]["First Name"], "Lisa");
    assert_equal(doc[3]["Last Name"], "Simpson");
}

TXL_RUN_TESTS()
