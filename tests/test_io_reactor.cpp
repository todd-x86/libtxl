#include <txl/unit_test.h>
#include <txl/io_reactor.h>
#include <txl/file.h>
#include <txl/socket.h>

auto fill_buffer(txl::io_reactor & io, txl::file & f, std::vector<std::byte> & out) -> void;

auto fill_buffer(txl::io_reactor & io, txl::file & f, std::vector<std::byte> & out) -> void
{
    io.read_async(f, 512, [&](auto buf) {
        fill_buffer(io, f, out);
    });
}

TXL_UNIT_TEST(TestIoReactor)
{
    auto io = txl::io_reactor{};
    io.start();

    auto f = txl::file{"../include/txl/memory_pool.h", "r"};
    //auto s = txl::tcp_socket{};

    //auto buf = co_await io.read_async(f, 64);

    io.read_async(f, 64, [](auto buf) {
        
    });

    io.stop();
    io.join();
}
