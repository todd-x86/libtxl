#include <txl/unit_test.h>
#include <txl/linked_list.h>

#include <sstream>

TXL_UNIT_TEST(atomic_linked_list_simple)
{
    auto l = txl::atomic_linked_list<int>{};
    assert_true(l.empty());
}

TXL_UNIT_TEST(atomic_linked_list_push_pop)
{
    auto l = txl::atomic_linked_list<int>{};
    l.emplace_back(1);
    l.emplace_back(2);
    l.emplace_back(3);
    assert_false(l.empty());
    assert_equal(*l.pop_and_release_front(), 1);
    assert_equal(*l.pop_and_release_front(), 2);
    assert_equal(*l.pop_and_release_front(), 3);
    assert_true(l.empty());
}

static size_t num_deletes = 0;

struct delete_me final
{
    ~delete_me()
    {
        ++num_deletes;
    }
};

TXL_UNIT_TEST(atomic_linked_list_free)
{
    auto l = txl::atomic_linked_list<delete_me>{};
    assert_equal(num_deletes, 0);
    l.emplace_back();
    l.emplace_back();
    l.emplace_back();
    assert_equal(num_deletes, 0);
    assert_false(l.empty());
    // pop_and_release_front() can destruct 3 instances (1 for empty node destruction, 1 for moving into optional, 1 for destructing the optional below (RAII))
    l.pop_and_release_front();
    assert_equal(num_deletes, 1*3);
    l.pop_and_release_front();
    assert_equal(num_deletes, 2*3);
    l.pop_and_release_front();
    assert_equal(num_deletes, 3*3);
    l.pop_and_release_front();
    assert_equal(num_deletes, 3*3);
    assert_true(l.empty());
}

TXL_UNIT_TEST(atomic_linked_list_move)
{
    auto l = txl::atomic_linked_list<std::string>{};
    l.emplace_back("Hello World No Small String Optimization Here");
    assert_false(l.empty());
    auto s = std::move(*l.pop_and_release_front());
    assert_true(l.empty());
    assert_equal(s, "Hello World No Small String Optimization Here");
}

TXL_UNIT_TEST(atomic_linked_list_add_lots)
{
    auto l = txl::atomic_linked_list<std::string>{};
    auto ss = std::ostringstream{};
    for (auto i = 0; i < 10000; ++i)
    {
        ss.str("");
        ss << "Hello from the following number: " << i;
        l.emplace_back(ss.str());
    }
    
    for (auto i = 0; i < 10000; ++i)
    {
        ss.str("");
        ss << "Hello from the following number: " << i;
        assert_equal(*l.pop_and_release_front(), ss.str());
    }
    assert_true(l.empty());
}

TXL_RUN_TESTS()
