#include <txl/unit_test.h>
#include <txl/overload.h>

#include <variant>

TXL_UNIT_TEST(overload)
{
    std::variant<int, double, std::string> example{};
    example = "foo";

    std::visit(
        txl::overload {
            [](int x) { std::cout << "AN INT" << std::endl; },
            [](double x) { std::cout << "A DOUBLE" << std::endl; },
            [](std::string x) { std::cout << "A STRING" << std::endl; }
        },
        example
    );
}

TXL_RUN_TESTS()
