#include <txl/event_poller.h>
#include <txl/pipe.h>
#include <txl/unit_test.h>

#include <string_view>

TXL_UNIT_TEST(TestOpenClose)
{
    auto p = txl::event_poller{};

    assert_false(p.is_open());
    
    p.open();
    assert_true(p.is_open());

    p.close();
    assert_false(p.is_open());
}

TXL_UNIT_TEST(TestPollEmpty)
{
    auto p = txl::event_poller{};
    auto events = txl::event_array<10>{};
    
    txl::system_error err{};
    p.poll(events, std::chrono::milliseconds{0}, txl::on_error::capture(err));
    assert_equal(err.code(), EBADF);

    p.open();
    auto num_available = p.poll(events, std::chrono::milliseconds{0});
    assert_equal(num_available, 0);
}

TXL_UNIT_TEST(TestPipe)
{
    auto p = txl::event_poller{};
    auto events = txl::event_array<10>{};
    
    p.open();

    auto c = txl::pipe_connector{};
    c.open();
    assert_true(c.input().is_open());
    assert_true(c.output().is_open());
    p.add(c.input().fd(), txl::event_type::in);

    auto s = std::string_view{"Hello"};
    auto buf = txl::buffer_ref{s};
    c.output().write(buf);
        
    auto num_available = p.poll(events, std::chrono::milliseconds{0});
    assert_equal(num_available, 1);
    assert_equal(events[0].events(), txl::event_type::in);
    assert_equal(events[0].fd(), c.input().fd());
}
/*
TXL_UNIT_TEST(TestAdd)
{
    auto p = txl::event_poller{};
    
    {
        auto res = p.open();
        assert_equal(res.status_code(), hpi::status::ok);
        assert_true(p.is_open());
    }

    auto c = hpi::system::pipe_connector{};
    {
        auto res = c.open();
        assert_equal(res.status_code(), hpi::status::ok);
        assert_true(c.input().is_open());
        assert_true(c.output().is_open());

        auto add_res = p.add(c.input().fd(), hpi::system::epoll::event_type::in);
        assert_true(add_res.ok());
        
        auto add_res2 = p.add(c.input().fd(), hpi::system::epoll::event_type::in);
        assert_false(add_res2.ok());
        assert_equal(add_res2.status_code(), hpi::status::system_error);
        assert_equal(add_res2.error(), hpi::system::error::already_exists);
    }
}

TXL_UNIT_TEST(TestModify)
{
    auto p = txl::event_poller{};
    
    {
        auto res = p.open();
        assert_equal(res.status_code(), hpi::status::ok);
        assert_true(p.is_open());
    }

    auto c = hpi::system::pipe_connector{};
    {
        auto res = c.open();
        assert_equal(res.status_code(), hpi::status::ok);
        assert_true(c.input().is_open());
        assert_true(c.output().is_open());

        auto add_res = p.add(c.input().fd(), hpi::system::epoll::event_type::in);
        assert_true(add_res.ok());

        auto mod_res = p.modify(c.input().fd(), hpi::system::epoll::event_type::out);
        assert_true(mod_res.ok());
        
        mod_res = p.modify(0, hpi::system::epoll::event_type::out);
        assert_false(mod_res.ok());
        assert_equal(mod_res.status_code(), hpi::status::system_error);
        assert_equal(mod_res.error(), hpi::system::error::no_entry);
    }
}

TXL_UNIT_TEST(TestRemove)
{
    auto p = txl::event_poller{};
    
    {
        auto res = p.open();
        assert_equal(res.status_code(), hpi::status::ok);
        assert_true(p.is_open());
    }

    auto c = hpi::system::pipe_connector{};
    {
        auto res = c.open();
        assert_equal(res.status_code(), hpi::status::ok);
        assert_true(c.input().is_open());
        assert_true(c.output().is_open());

        auto add_res = p.add(c.input().fd(), hpi::system::epoll::event_type::in);
        assert_true(add_res.ok());

        auto rem_res = p.remove(c.input().fd());
        assert_true(rem_res.ok());
        
        rem_res = p.remove(c.input().fd());
        assert_false(rem_res.ok());
        assert_equal(rem_res.status_code(), hpi::status::system_error);
        assert_equal(rem_res.error(), hpi::system::error::no_entry);
    }
}
*/

TXL_RUN_TESTS()
