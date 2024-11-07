#pragma once

#include <txl/buffer_ref.h>
#include <txl/memory_pool.h>
#include <txl/socket.h>
#include <txl/on_error.h>

#include <functional>
#include <list>
#include <vector>

namespace txl
{
    using io_event = std::function<void(buffer_ref)>;

    struct io_reactor_params final
    {
        size_t in_buffer_page_size_;
        size_t in_buffer_num_pages_;
        size_t out_buffer_page_size_;
        size_t out_buffer_num_pages_;
    };

    class io_reactor
    {
    private:
        struct io_event_data final
        {
            io_event on_io_;
        };

        memory_pool in_buf_;
        memory_pool out_buf_;
        event_poller poller_{};
        std::list<io_event> events_;
    public:
        io_reactor(io_reactor_params && params)
            : in_buf_(params.in_buffer_num_pages_, params.in_buffer_page_size_)
            , out_buf_(params.out_buffer_num_pages_, params.out_buffer_page_size_)
        {
        }

        auto add_reader(int fd, io_event on_data, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            events_.push_back(on_data);
            poller_.add(fd, event_poller::in | event_poller::out | event_poller::error, &events_.back(), on_err);
        }

        auto begin_write(size_t num_bytes) -> buffer_ref
        {
            
        }
    };
}
