#include <txl/unit_test.h>
#include <txl/tar_file.h>
#include <txl/file.h>

TXL_UNIT_TEST(simple_tar)
{
    txl::file inp_file{"./test1.tar", "r"};
    txl::tar_reader rd{inp_file};
    while (true)
    {
        auto pending = rd.read_entry();
        if (pending.empty())
        {
            break;
        }
        auto e = pending.or_throw();
        std::cout << '"' << e.filename() << '"' << " -- > " << e.size() << std::endl;
    }
}

TXL_RUN_TESTS()
