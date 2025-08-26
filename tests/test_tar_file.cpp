#include <txl/unit_test.h>
#include <txl/tar_file.h>
#include <txl/file.h>
#include <txl/time.h>
#include <txl/foreach_result.h>

#include <string>
#include <chrono>
#include <ostream>
#include <optional>
#include <vector>

struct test_tar_data final
{
    std::string filename;
    std::chrono::system_clock::time_point mod_time;
    txl::tar_entry::entry_type type;
    size_t size;
    std::optional<std::string> contents;

    test_tar_data(std::string const & filename, std::chrono::system_clock::time_point mod_time, txl::tar_entry::entry_type type, size_t size = 0, std::optional<std::string> const & contents = {})
        : filename{filename}
        , mod_time{mod_time}
        , type{type}
        , size{size}
        , contents{contents}
    {
    }
    
    auto operator==(test_tar_data const & o) const -> bool
    {
        return filename == o.filename and mod_time == o.mod_time and type == o.type and size == o.size and contents == o.contents;
    }
};

inline auto operator<<(std::ostream & os, test_tar_data const & d) -> std::ostream &
{
    os << "{filename=" << d.filename << ", mod_time=" << txl::time::format_time_point(d.mod_time) << ", type=" << d.type << ", size=" << d.size << ", contents=" << (d.contents ? *d.contents : "") << "}";
    return os;
}

inline auto get_time_point(std::string const & s) -> std::chrono::system_clock::time_point
{
    return txl::time::to_time_point_utc(s, "%Y%m%d-%H:%M:%S");
}

TXL_UNIT_TEST(simple_tar)
{
    std::vector<test_tar_data> expected{}, actual{};

    expected.emplace_back(
        "tar_example/",
        get_time_point("20250816-02:26:52"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/",
        get_time_point("20250816-02:27:01"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/",
        get_time_point("20250816-02:27:06"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/",
        get_time_point("20250816-02:27:12"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/maybe/",
        get_time_point("20250816-02:27:11"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/not_sure/",
        get_time_point("20250816-02:27:18"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/not_sure/if_not_why_not/",
        get_time_point("20250816-02:27:42"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/not_sure/if_not_why_not/is_it_long_enough_now/",
        get_time_point("20250816-02:27:47"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/not_sure/if_not_why_not/is_it_long_enough_now/how_about_now/",
        get_time_point("20250816-02:27:51"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/not_sure/if_not_why_not/is_it_long_enough_now/how_about_now/okay_maybe/",
        get_time_point("20250816-02:28:18"),
        txl::tar_entry::directory
    );
    expected.emplace_back(
        "tar_example/toolongfortar/this_is_too_long/is_this_too_long/not_sure/if_not_why_not/is_it_long_enough_now/how_about_now/okay_maybe/README.md",
        get_time_point("20250816-02:28:17"),
        txl::tar_entry::file,
        17,
        std::optional<std::string>{"can you read me?\n"}
    );
    
    txl::file inp_file{"tar_data/test1.tar", "r"};
    txl::tar_reader rd{inp_file};
    FOREACH_RESULT(rd.read_entry(), e)
    {
        std::optional<std::string> contents{};
        if (e.type() == txl::tar_entry::file)
        {
            auto data_rdr = e.open_data_reader(inp_file);
            contents = txl::read_string(data_rdr, e.size()).or_throw();
        }

        actual.emplace_back(std::string{e.filename()}, e.modification_time(), e.type(), e.size(), contents);
    }

    assert_equal(expected, actual);
}

TXL_RUN_TESTS()
