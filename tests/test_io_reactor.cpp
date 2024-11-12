#include <txl/unit_test.h>
#include <txl/io_reactor.h>
#include <txl/file.h>
#include <txl/socket.h>
#include <txl/app.h>

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

    auto f = txl::file{(txl::get_application_path() / "io_reactor_data/sample.txt").string(), "r"};
    //auto s = txl::tcp_socket{};

    //auto buf = co_await io.read_async(f, 64);

    auto res = io.read_async(f, f.seekable_size(), [](auto buf) {
        //std::cout << "GOT DATA " << buf.size() << " bytes" << std::endl;
    });

    res.wait();

    io.stop();
    io.join();
}

TXL_RUN_TESTS()
