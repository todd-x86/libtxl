#pragma once

#include <txl/buffer_ref.h>
#include <txl/event_poller.h>
#include <txl/memory_pool.h>
#include <txl/socket.h>
#include <txl/file.h>
#include <txl/on_error.h>

#include <condition_variable>
#include <functional>
#include <thread>

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
        memory_pool buf_in_;
        event_poller poller_;
        std::thread io_thread_{};

        auto io_thread_loop() -> void
        {
            auto ev = txl::event_vector{30};
            while (true)
            {
                auto num_available = poller_.poll(ev);
                if (num_available > 0)
                {
                    //for (
                }
            }
        }
    public:
        io_reactor()
            : buf_in_(16, 4096)
        {
        }

        auto start() -> void
        {
            //if (
        }

        auto stop() -> void
        {
        }

        auto join() -> void
        {
        }

        auto read_async(txl::file & file, size_t num_bytes, io_event on_data) -> void
        {
        }
    };
}
