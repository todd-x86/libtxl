#include <txl/event_poller.h>
#include <txl/pipe.h>
#include <txl/unit_test.h>

#include <string_view>

TXL_UNIT_TEST(event_poller_open_close)
{
    auto p = txl::event_poller{};

    assert_false(p.is_open());
    
    p.open().or_throw();
    assert_true(p.is_open());

    p.close().or_throw();
    assert_false(p.is_open());
}

TXL_UNIT_TEST(event_poller_empty)
{
    auto p = txl::event_poller{};
    auto events = txl::event_array<10>{};
    
    auto err = p.poll(events, std::chrono::milliseconds{0});
    assert_true(err.is_error());
    assert_equal(err.error().value(), EBADF);

    p.open().or_throw();
    auto num_available = p.poll(events, std::chrono::milliseconds{0}).or_throw();
    assert_equal(num_available, 0);
}

TXL_UNIT_TEST(event_poller_pipe)
{
    auto p = txl::event_poller{};
    auto events = txl::event_array<10>{};
    
    p.open().or_throw();

    auto c = txl::pipe_connector{};
    c.open().or_throw();
    assert_true(c.input().is_open());
    assert_true(c.output().is_open());
    p.add(c.input().fd(), txl::event_type::in).or_throw();

    auto s = std::string_view{"Hello"};
    auto buf = txl::buffer_ref{s};
    c.output().write(buf).or_throw();
        
    auto num_available = p.poll(events, std::chrono::milliseconds{0}).or_throw();
    assert_equal(num_available, 1);
    assert_equal(events[0].events(), txl::event_type::in);
    assert_equal(events[0].fd(), c.input().fd());
}

TXL_UNIT_TEST(event_poller_add)
{
    auto p = txl::event_poller{};
    
    {
        p.open().or_throw();
        assert_true(p.is_open());
    }

    auto c = txl::pipe_connector{};
    {
        c.open().or_throw();
        assert_true(c.input().is_open());
        assert_true(c.output().is_open());

        auto add_res = p.add(c.input().fd(), txl::event_type::in);
        assert_true(add_res);
        
        auto add_res2 = p.add(c.input().fd(), txl::event_type::in);
        assert_false(add_res2);
        assert_equal(add_res2.error(), txl::get_system_error(EEXIST));
    }
}

TXL_UNIT_TEST(event_poller_modify)
{
    auto p = txl::event_poller{};
    
    {
        auto res = p.open();
        assert_false(res.is_error());
        assert_true(p.is_open());
    }

    auto c = txl::pipe_connector{};
    {
        auto res = c.open();
        assert_false(res.is_error());
        assert_true(c.input().is_open());
        assert_true(c.output().is_open());

        auto add_res = p.add(c.input().fd(), txl::event_type::in);
        assert_false(add_res.is_error());

        auto mod_res = p.modify(c.input().fd(), txl::event_type::out);
        assert_false(mod_res.is_error());
        
        mod_res = p.modify(0, txl::event_type::out);
        assert_true(mod_res.is_error());
        assert_equal(mod_res.error(), txl::get_system_error(ENOENT));
    }
}

TXL_UNIT_TEST(event_poller_remove)
{
    auto p = txl::event_poller{};
    
    {
        auto res = p.open();
        assert_false(res.is_error());
        assert_true(p.is_open());
    }

    auto c = txl::pipe_connector{};
    {
        auto res = c.open();
        assert_false(res.is_error());
        assert_true(c.input().is_open());
        assert_true(c.output().is_open());

        auto add_res = p.add(c.input().fd(), txl::event_type::in);
        assert_false(add_res.is_error());

        auto rem_res = p.remove(c.input().fd());
        assert_false(rem_res.is_error());
        
        rem_res = p.remove(c.input().fd());
        assert_true(rem_res.is_error());
        assert_equal(rem_res.error(), txl::get_system_error(ENOENT));
    }
}

TXL_RUN_TESTS()
