#include <txl/unit_test.h>
#include <txl/socket.h>

TXL_UNIT_TEST(socket)
{
    txl::socket s{};
    assert_false(s.is_open());
}

TXL_UNIT_TEST(socket_bind)
{
    txl::socket s{txl::socket::internet, txl::socket::stream};
    txl::socket_address sa{};
    s.bind(sa);
}

TXL_RUN_TESTS()
