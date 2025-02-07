#include <txl/ring_buffer_file.h>
#include <iostream>

int main(int argc, char * argv[])
{
    txl::ring_buffer_file rb{argv[1], txl::ring_buffer_file::read_only, 4096 * 64};
    while (true)
    {
        auto c = rb.cursor_internal();
        auto f = rb.cursor_file();
        auto r = rb.read().or_throw();
        if (not r.empty())
        {
            if (r.size() > 27)
            {
                std::cout << r.size() << " bytes (I=" << c << ", F=" << f << ")" << std::endl;
            }
            std::cout << r.size() << " bytes\n";
            //std::cout << r.to_string_view() << std::endl;
        }
    }
    return 0;
}
