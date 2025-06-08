#include <txl/unit_test.h>
#include <txl/socket.h>
#include <txl/read_string.h>
#include <txl/time.h>

#include <string_view>

TXL_UNIT_TEST(socket)
{
    txl::socket s{};
    assert_false(s.is_open());
}

TXL_UNIT_TEST(socket_bind_listen)
{
    // Open up random listener
    txl::socket server{txl::socket::internet, txl::socket::stream};
    txl::socket_address server_addr{"127.0.0.1"};
    server.bind(server_addr).or_throw();
    server.listen(0).or_throw();

    // Connect to listener
    txl::socket_address client_addr{"127.0.0.1", server.get_local_address().or_throw().port()};
    txl::socket client{txl::socket::internet, txl::socket::stream};
    client.set_option(txl::socket_option::recv_timeout, txl::time::to_timeval(std::chrono::seconds{1})).or_throw(); 
    client.set_option(txl::socket_option::send_timeout, txl::time::to_timeval(std::chrono::seconds{1})).or_throw(); 
    client.connect(client_addr).or_throw();
}

TXL_UNIT_TEST(socket_accept)
{
    // Open up random listener
    txl::socket server{txl::socket::internet, txl::socket::stream};
    txl::socket_address server_addr{"127.0.0.1"};
    server.bind(server_addr).or_throw();
    server.listen(0).or_throw();

    // Connect to listener
    txl::socket_address client_addr{"127.0.0.1", server.get_local_address().or_throw().port()};
    txl::socket client{txl::socket::internet, txl::socket::stream};
    client.connect(client_addr).or_throw();

    // Accept client
    auto server_client = server.accept().or_throw();

    assert_equal(client.get_remote_address().or_throw().port(), server.get_local_address().or_throw().port());
    assert_equal(server_client.get_local_address().or_throw().port(), server.get_local_address().or_throw().port());
}

TXL_UNIT_TEST(socket_read_write)
{
    using namespace std::literals;

    // Open up random listener
    txl::socket server{txl::socket::internet, txl::socket::stream};
    txl::socket_address server_addr{"127.0.0.1"};
    server.bind(server_addr).or_throw();
    server.listen(0).or_throw();

    // Connect to listener
    txl::socket_address client_addr{"127.0.0.1", server.get_local_address().or_throw().port()};
    txl::socket client{txl::socket::internet, txl::socket::stream};
    client.connect(client_addr).or_throw();

    auto server_client = server.accept().or_throw();

    server_client.write("Hello World"sv).or_throw();
    auto hello = txl::read_string(client, txl::at_most{200}).or_throw();

    assert_equal("Hello World"sv, std::string_view{hello});
}

TXL_RUN_TESTS()
