#include <txl/unit_test.h>
#include <txl/socket.h>
#include <txl/read_string.h>

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
    server.bind(server_addr);
    server.listen(0);

    // Connect to listener
    txl::socket_address client_addr{"127.0.0.1", server.get_local_address().port()};
    txl::socket client{txl::socket::internet, txl::socket::stream};
    client.connect(client_addr);
}

TXL_UNIT_TEST(socket_accept)
{
    // Open up random listener
    txl::socket server{txl::socket::internet, txl::socket::stream};
    txl::socket_address server_addr{"127.0.0.1"};
    server.bind(server_addr);
    server.listen(0);

    // Connect to listener
    txl::socket_address client_addr{"127.0.0.1", server.get_local_address().port()};
    txl::socket client{txl::socket::internet, txl::socket::stream};
    client.connect(client_addr);

    // Accept client
    auto server_client = server.accept();

    assert_equal(client.get_remote_address().port(), server.get_local_address().port());
    assert_equal(server_client.get_local_address().port(), server.get_local_address().port());
}

TXL_UNIT_TEST(socket_read_write)
{
    using namespace std::literals;

    // Open up random listener
    txl::socket server{txl::socket::internet, txl::socket::stream};
    txl::socket_address server_addr{"127.0.0.1"};
    server.bind(server_addr);
    server.listen(0);

    // Connect to listener
    txl::socket_address client_addr{"127.0.0.1", server.get_local_address().port()};
    txl::socket client{txl::socket::internet, txl::socket::stream};
    client.connect(client_addr);

    auto server_client = server.accept();

    server_client.write("Hello World"sv);
    auto hello = txl::read_string(client, txl::at_most{200});

    assert_equal("Hello World"sv, std::string_view{hello});
}

TXL_RUN_TESTS()
