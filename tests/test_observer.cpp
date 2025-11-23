#include <txl/unit_test.h>
#include <txl/observer.h>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <string>

static size_t num_expensive_messages_created = 0;

namespace msgs
{
    struct memory_usage final
    {
        size_t num_bytes_used, num_bytes_total;
    };

    struct debug_line final
    {
        std::string msg;
    };

    struct expensive_message final
    {
        expensive_message()
        {
            ++num_expensive_messages_created;
        }
    };
}

struct test_observer
{
    std::vector<msgs::memory_usage> memory_usage;
    std::vector<msgs::debug_line> debug_line;
    std::vector<msgs::expensive_message> expensive_message;

    auto clear()
    {
        memory_usage.clear();
        debug_line.clear();
        expensive_message.clear();
    }

    auto num_bytes_used() const -> size_t
    {
        return std::accumulate(memory_usage.begin(), memory_usage.end(), 0, [](auto v, auto next) { return v + next.num_bytes_used; });
    }

    auto on_message(msgs::memory_usage const & m) -> bool
    {
        memory_usage.emplace_back(m);
        return true;
    }

    auto on_message(msgs::debug_line const & m) -> bool
    {
        debug_line.emplace_back(m);
        return true;
    }

    auto on_message(msgs::expensive_message const & m) -> bool
    {
        expensive_message.push_back(m);
        return true;
    }
};

TXL_UNIT_TEST(emit_and_observe)
{
    test_observer ob{};
    OBSERVE(ob, msgs::memory_usage, msgs::debug_line, msgs::expensive_message);
    EMIT((msgs::memory_usage{1,10}));
    EMIT((msgs::memory_usage{1,10}));
    EMIT((msgs::memory_usage{1,10}));
    UNOBSERVE(ob);

    assert_equal(ob.num_bytes_used(), 3);
}

TXL_UNIT_TEST(observe_and_unobserve)
{
    test_observer ob{};
    OBSERVE(ob, msgs::memory_usage);
    EMIT((msgs::memory_usage{1,20}));
    EMIT((msgs::memory_usage{1,20}));
    UNOBSERVE(ob);
    EMIT((msgs::memory_usage{1,20}));

    assert_equal(ob.num_bytes_used(), 2);
}

TXL_UNIT_TEST(observe_specific)
{
    test_observer ob1{}, ob2{};
    OBSERVE(ob1, msgs::memory_usage, msgs::debug_line);
    OBSERVE(ob2, msgs::expensive_message, msgs::debug_line);
    EMIT((msgs::memory_usage{1,20}));
    EMIT(msgs::expensive_message{});
    EMIT(msgs::debug_line{"Not the comfy chair!"});
    UNOBSERVE(ob1);
    UNOBSERVE(ob2);

    assert_equal(ob1.memory_usage.size(), 1);
    assert_equal(ob1.debug_line.size(), 1);
    assert_equal(ob1.debug_line[0].msg, "Not the comfy chair!");
    assert_equal(ob2.expensive_message.size(), 1);
    assert_equal(ob2.debug_line.size(), 1);
    assert_equal(ob2.debug_line[0].msg, "Not the comfy chair!");
}

TXL_UNIT_TEST(lazy_emit)
{
    test_observer ob{};
    OBSERVE(ob, msgs::expensive_message);

    num_expensive_messages_created = 0;
    LAZY_EMIT(msgs::expensive_message{});

    assert_equal(num_expensive_messages_created, 1);
    assert_equal(ob.expensive_message.size(), num_expensive_messages_created);
    UNOBSERVE(ob);
    
    LAZY_EMIT(msgs::expensive_message{});

    assert_equal(num_expensive_messages_created, 1);
    assert_equal(ob.expensive_message.size(), num_expensive_messages_created);
}

TXL_UNIT_TEST(new_observer)
{
    auto ob = std::make_shared<test_observer>();
    CONNECT_OBSERVER(ob);
    OBSERVE(ob, msgs::expensive_message);

    num_expensive_messages_created = 0;
    LAZY_EMIT(msgs::expensive_message{});

    assert_equal(num_expensive_messages_created, 1);
    assert_equal(ob->expensive_message.size(), num_expensive_messages_created);
    DISCONNECT_OBSERVER(ob);
    UNOBSERVE(ob);
    
    LAZY_EMIT(msgs::expensive_message{});

    assert_equal(num_expensive_messages_created, 1);
    assert_equal(ob->expensive_message.size(), num_expensive_messages_created);
}

TXL_RUN_TESTS()
