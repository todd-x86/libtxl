#include <txl/unit_test.h>
#include <txl/tar_file.h>
#include <txl/file.h>
#include <txl/foreach_result.h>

TXL_UNIT_TEST(simple_tar)
{
    txl::file inp_file{"./test1.tar", "r"};
    txl::tar_reader rd{inp_file};
    FOREACH_RESULT(rd.read_entry(), e)
    {
        std::cout << txl::time::format_time_point(e.modification_time()) << '"' << e.filename() << '"' << " -- > " << e.size() << " [" << (e.type() == txl::tar_entry::directory ? "dir" : "file") << "] " << std::endl;
        if (e.type() == txl::tar_entry::file)
        {
            auto data_rdr = e.open_data_reader(inp_file);
            auto data = txl::read_string(data_rdr, e.size());
            std::cout << "\"" << *data << "\" " << e.size() << std::endl;
        }
    }
}

TXL_RUN_TESTS()
