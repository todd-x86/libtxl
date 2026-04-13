#pragma once

#include <txl/socket.h>
#include <txl/event_poller.h>
#include <txl/buffer_ref.h>
#include <txl/box.h>
#include <txl/flat_map.h>
#include <txl/patterns.h>

namespace txl::events
{
    class event_loop;

    struct event_base
    {
        virtual auto on_event_added(event_loop & el) -> void = 0;
        virtual auto on_event_removed(event_loop & el) -> void = 0;

        virtual auto fd() const -> int = 0;
    };

    struct data_received_event
    {
        virtual auto on_data_received() -> bool
        {
            return true;
        }
    };

    struct data_sent_event
    {
        virtual auto on_data_sent() -> bool
        {
            return true;
        }
    };

    class socket_event : public event_base
    {
    private:
        ::txl::socket sock_;
    protected:
        auto socket() -> ::txl::socket & { return sock_; }
        auto socket() const -> ::txl::socket const & { return sock_; }
    public:
        auto fd() const final -> int { return sock_.fd(); }
    };

    class event_loop
    {
    private:
        ::txl::event_poller poller_{};
        ::txl::flat_map<int, ::txl::box<event_base>> fd_to_event_;
    public:
        auto add(::txl::box<event_base> e) -> bool
        {
            if (not ::txl::try_emplace(fd_to_event_, e->fd(), [&] { return std::move(e); }))
            {
                return false;
            }

            e->on_event_added(*this);
            return true;
        }
    };
}
