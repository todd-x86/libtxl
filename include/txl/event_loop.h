#pragma once

#include <txl/event_timer.h>
#include <txl/socket.h>
#include <txl/event_poller.h>
#include <txl/buffer_ref.h>
#include <txl/box.h>
#include <txl/flat_map.h>
#include <txl/patterns.h>
#include <txl/result.h>

#include <chrono>
#include <optional>

namespace txl::events
{
    class event_loop;

    struct event_base
    {
        virtual auto on_event_added(event_loop & el) -> void = 0;
        virtual auto on_event_removed(event_loop & el) -> void = 0;
        
        virtual auto on_data_received() -> bool
        {
            return true;
        }
        
        virtual auto on_data_sent() -> bool
        {
            return true;
        }

        virtual auto fd() const -> int = 0;
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

    class timer_event : public event_base
    {
    private:
        ::txl::event_timer timer_;
    protected:
        auto timer() -> ::txl::event_timer & { return timer_; } 
        auto timer() const -> ::txl::event_timer const & { return timer_; }

        timer_event(::txl::event_timer::timer_type tt)
            : timer_{tt}
        {
        }
    public:
        auto fd() const final -> int { return timer_.fd(); }
    };

    struct dispatch_stats
    {
        std::chrono::nanoseconds wait_time = {0};
        size_t num_dispatched = 0;
    };

    struct dispatch_params
    {
        std::optional<std::chrono::milliseconds> timeout;
        std::optional<size_t> min_events, max_events;
    };

    class event_loop
    {
    private:
        ::txl::event_poller poller_{};
        ::txl::event_vector evts_{32};
        ::txl::flat_map<int, ::txl::box<event_base>> fd_to_event_;
    public:
        auto add(::txl::box<event_base> e, ::txl::event_type ev_type) -> ::txl::result<bool>
        {
            if (auto [p, emplaced] = ::txl::emplace(fd_to_event_, e->fd(), [&] { return std::move(e); }); emplaced)
            {
                auto add_res = poller_.add((*p)->fd(), ev_type, ::txl::event_tag::from_ptr(p->get()));
                if (add_res.is_error())
                {
                    return add_res.error();
                }
                (*p)->on_event_added(*this);
                return true;
            }
            return false;
        }

        auto dispatch(dispatch_params const & params) -> ::txl::result<dispatch_stats>
        {
            auto t1 = std::chrono::steady_clock::now();
            auto res = poller_.poll(evts_, timeout);
            auto t2 = std::chrono::steady_clock::now();

            if (res.is_error())
            {
                return res.error();
            }

            
        }
    };
}
