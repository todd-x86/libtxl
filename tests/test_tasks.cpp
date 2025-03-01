#include <txl/unit_test.h>
#include <txl/tasks.h>

#include <sstream>

TXL_UNIT_TEST_VARIATION(inline_runner, []() {
    txl::task_runner::set_global(std::make_unique<txl::inline_task_runner>());
});

TXL_UNIT_TEST_VARIATION(thread_pool_runner, []() {
    txl::task_runner::set_global(std::make_unique<txl::thread_pool_task_runner>(4));
});

TXL_UNIT_TEST(simple)
{
    auto num_calls = 0;
    auto ss = std::ostringstream{};
    auto workflow = txl::make_task<void>([&]() {
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
    auto workflow = txl::make_task<void>([&]() {
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
    auto unit_of_work = txl::make_task<void>([&num_calls]() {
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
    auto after = std::chrono::steady_clock::time_point{};

    auto sleep_then_work = txl::task<void>([&before]() {
        before = std::chrono::steady_clock::now();
    })
    .then(txl::delay(std::chrono::milliseconds(1)))
    .then([&after]() {
        after = std::chrono::steady_clock::now();
    });

    sleep_then_work();

    assert_true(std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count() >= 1);
}

TXL_UNIT_TEST(in_order)
{
    std::vector<std::string> secret_message{};

    auto chain1 = txl::make_task<void>([&secret_message]() {
        secret_message.emplace_back("Be");
    }).then([&]() {
        secret_message.emplace_back("sure");
    });
    
    chain1();
    assert_equal(secret_message, std::vector<std::string>{"Be","sure"});
    secret_message.clear();
    
    auto chain2 = txl::make_task<void>([&secret_message]() {
        secret_message.emplace_back("your");
    }).then([&]() {
        secret_message.emplace_back("Ovaltine!");
    });
    
    auto chain3 = txl::make_task<void>([&secret_message]() {
        secret_message.emplace_back("to");
    }).then([&]() {
        secret_message.emplace_back("drink");
    });

    chain1.then(std::move(chain3)).then(std::move(chain2));

    
    assert_equal(secret_message, std::vector<std::string>{});
    
    chain1();
    
    assert_equal(secret_message, std::vector<std::string>{"Be","sure","to","drink","your","Ovaltine!"});
}

TXL_UNIT_TEST(then_after_nothing)
{
    auto called = false;
    txl::task<void> nothing{};

    nothing.then([&]() {
        called = true;
    });

    nothing();
    assert_true(called);
}

TXL_UNIT_TEST(something_then_nothing)
{
    auto called = false;
    auto num_calls = 0;
    txl::task<void> nothing{};
    txl::task<void> something([&]() {
        called = true;
        ++num_calls;
    });
    txl::task<void> something_again([&]() {
        ++num_calls;
    });
    something.then(std::move(nothing)).then(std::move(something_again));

    something();
    assert_true(called);
    assert_equal(num_calls, 2);
}

static auto is_prime(int s) -> bool
{
    for (auto i = 2; i <= (s >> 1); ++i)
    {
        if (s % i == 0)
        {
            return false;
        }
    }
    return true;
}

template<class T>
struct safe_vector final
{
private:
    std::vector<T> data_{};
    std::mutex mut_;
public:
    template<class... Args>
    auto emplace_back(Args && ... args)
    {
        std::unique_lock<std::mutex> lk{mut_};
        data_.emplace_back(std::forward<Args>(args)...);
    }

    auto size() const -> size_t { return data_.size(); }

    auto copy() const -> std::vector<T>
    {
        return data_;
    }
};


TXL_UNIT_TEST(run_in_parallel)
{
    safe_vector<int> primes{};
    std::vector<txl::task<void>> tasks{};

    for (auto i = 0; i < 100; ++i)
    {
        auto start = i * 1000;
        auto end = (i + 1) * 1000;
        auto t = txl::task<void>{[&primes, start, end]() {
            for (auto i = start; i < end; ++i)
            {
                if (is_prime(i))
                {
                    primes.emplace_back(i);
                }
            }
        }};
        tasks.emplace_back(std::move(t));
    }

    auto c1 = std::chrono::high_resolution_clock::now();
    for (auto & t : tasks)
    {
        t.get_promise().reset();
        t.run(txl::task_runner::global());
    }
    for (auto & t : tasks)
    {
        t.wait();
    }
    auto c2 = std::chrono::high_resolution_clock::now();

    std::cout << "TIME: " << std::chrono::duration_cast<std::chrono::microseconds>(c2-c1) << " | " << primes.size() << std::endl;
    assert_equal(primes.size(), 9594);
}

TXL_RUN_TESTS()
