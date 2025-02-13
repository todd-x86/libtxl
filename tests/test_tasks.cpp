#include <txl/unit_test.h>
#include <txl/tasks.h>

#include <sstream>

TXL_UNIT_TEST(simple)
{
    auto num_calls = 0;
    auto ss = std::ostringstream{};
    auto workflow = txl::make_task([&]() {
        ++num_calls;
        ss << "Hello ";
    }).then([&]() {
        ++num_calls;
        ss << "World";
    });

    workflow();

    assert_equal(num_calls, 2);
    assert_equal(ss.str(), "Hello World");
}

TXL_UNIT_TEST(simple_with_context)
{
    auto num_calls = 0;
    auto num_continuations = 0;
    auto ss = std::ostringstream{};
    auto workflow = txl::make_task([&]() {
        ++num_calls;
        ss << "Hello ";
    }).then([&](auto & context) {
        if (context.is_success())
        {
            ++num_continuations;
        }
        ++num_calls;
        ss << "Cat";
    });

    workflow();

    assert_equal(num_calls, 2);
    assert_equal(num_continuations, 1);
    assert_equal(ss.str(), "Hello Cat");
}

TXL_UNIT_TEST(simple_return)
{
    auto get_meaning_of_life = txl::make_task<int>([]() {
        return 42;
    });
    
    auto meaning_of_life = get_meaning_of_life();
    assert_equal(meaning_of_life, 42);
}

TXL_UNIT_TEST(roll_call)
{
    auto robot_roll_call = txl::make_task<std::string>([]() {
        return "Cambot";
    }).then([](auto & ctx) {
        return ctx.result() + ", Gypsy";
    }).then([](auto & ctx) {
        return ctx.result() + ", Tom Servo";
    }).then([](auto & ctx) {
        return ctx.result() + ", Crow";
    });
    
    auto robots = robot_roll_call();
    assert_equal(robots, "Cambot, Gypsy, Tom Servo, Crow");
}

TXL_UNIT_TEST(counting)
{
    auto get_count = txl::make_task<int>([]() {
        return 1;
    });

    for (auto i = 0; i < 10; ++i)
    {
        get_count.then([](auto & ctx) {
            return ctx.result() + 1;
        });
    }
    
    auto count = get_count();
    assert_equal(count, 11);
}

TXL_UNIT_TEST(reusable)
{
    int num_calls = 0;
    auto unit_of_work = txl::make_task([&num_calls]() {
        ++num_calls;
    });

    for (auto i = 0; i < 10; ++i)
    {
        unit_of_work();
    }
    assert_equal(num_calls, 10);
}

TXL_UNIT_TEST(sleep)
{
    auto before = std::chrono::steady_clock::time_point{};
    auto sleep_then_work = txl::task<void>([&before]() {
        before = std::chrono::steady_clock::now();
    })
    .then(txl::tasks::sleep(std::chrono::milliseconds(1))
    .then([&after]() {
        after = std::chrono::steady_clock::now();
    });
}

TXL_RUN_TESTS()
