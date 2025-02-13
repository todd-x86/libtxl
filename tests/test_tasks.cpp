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

    txl::task_runner runner{std::make_unique<txl::inline_task_work_runner>()};
    runner.run(workflow);

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

    txl::task_runner runner{std::make_unique<txl::inline_task_work_runner>()};
    runner.run(workflow);

    assert_equal(num_calls, 2);
    assert_equal(num_continuations, 1);
    assert_equal(ss.str(), "Hello Cat");
}

TXL_RUN_TESTS()
