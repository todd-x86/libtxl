#include <txl/ring_buffer_file.h>
#include <iostream>
#include <sstream>

#include <cstdlib>

int main(int argc, char * argv[])
{
    std::ostringstream ss{};
    txl::ring_buffer_file rb{argv[1], txl::ring_buffer_file::read_write, /*4096 * 64*/ 4*1024*1024, 1024*1024};
    auto i = 0;
    while (true)
    {
        ss.str("");
        ss << "Hello from ID #" << i << " (r=" << rand() << ")!";
        ++i;
        rb.write(ss.str()).or_throw();
    }
    return 0;
}
