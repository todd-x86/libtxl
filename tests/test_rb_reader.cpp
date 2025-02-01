#include <txl/ring_buffer_file.h>
#include <iostream>

int main(int argc, char * argv[])
{
    txl::ring_buffer_file rb{argv[1], txl::ring_buffer_file::read_only, 4096 * 64};
    while (true)
    {
        auto r = rb.read().or_throw();
        if (not r.empty())
        {
            std::cout << r.to_string_view() << " -- H:" << rb.file_head() << ", T:" << rb.file_tail() << ", O:" << rb.offset() << std::endl;
        }
        else
        {
            std::cout << rb.offset() << std::endl;
        }
    }
    return 0;
}
