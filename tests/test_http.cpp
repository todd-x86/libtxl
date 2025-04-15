#include <txl/http.h>

int main()
{
    txl::http::test_server server{};
    server.run();
}
