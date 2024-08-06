#include <txl/unit_test.h>
#include <txl/fixed_string.h>

#include <array>

TXL_UNIT_TEST(fixed_string)
{
    txl::fixed_string<16> account{};
    assert(account == "");

    account = "Hello World";
    assert(account.size() == 11);
    assert(account == "Hello World");
    assert(account != "Hello Worl");
    assert(not (account != "Hello World"));

    account = "THIS IS A REALLY LONG STRING!";
    assert(account.size() == 16);
    assert(account == "THIS IS A REALLY");
    assert(account != "THIS IS A REALLY LONG STRING");
    
    account = "x";
    assert(account == "x");
    assert(account.size() == 1);
    
    account = "";
    assert(account == "");
    assert(account.size() == 0);
    
    txl::fixed_string<12> name{};
    name = "HELLO";
    assert(name == "HELLO");

    account = name;
    assert(account == "HELLO");
    
    name = account;
    assert(account == name);
    assert(name == "HELLO");
    assert(account == "HELLO");
    assert(account.to_string_view() == std::string_view{"HELLO"});
    assert(name.to_string_view() == account.to_string_view());

    assert(name[0] == 'H');
    assert(name[1] == 'E');
    assert(name[2] == 'L');
    assert(name[3] == 'L');
    assert(name[4] == 'O');

    name[0] = 'B';
    assert(name == "BELLO");

    txl::fixed_string<8> data{"12345678"};
    assert(data == "12345678");

    auto shakezula = std::string{"Master Shake"};
    txl::fixed_string<20> athf{shakezula};
    assert(athf == "Master Shake");

    using cartoon_name = txl::fixed_string<14>;
    std::array<cartoon_name, 4> aqua_teens {
        "Shake",
        "Frylock",
        "Meatwad",
        "Carl",
    };
    assert(aqua_teens[0] == "Shake");
    assert(aqua_teens[1] == "Frylock");
    assert(aqua_teens[2] == "Meatwad");
    assert(aqua_teens[3] == "Carl");
}

TXL_RUN_TESTS()
