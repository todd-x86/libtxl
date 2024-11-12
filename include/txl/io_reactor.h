#pragma once

#include <txl/buffer_ref.h>
#include <txl/event_poller.h>
#include <txl/memory_pool.h>
#include <txl/socket.h>
#include <txl/file.h>
#include <txl/on_error.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
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

    class io_completer final
    {
    private:
        std::mutex mutex_;
        std::condition_variable cond_;
    public:
        auto wait() -> void
        {
            std::unique_lock l{mutex_};
            cond_.wait(l);
        }

        auto set_complete() -> void
        {
            cond_.notify_all();
        }
    };

    class io_task final
    {
    private:
        io_completer & completer_;
    public:
        io_task(io_completer & completer)
            : completer_(completer)
        {
        }

        auto wait() -> void
        {
            completer_.wait();
        }
    };

    class io_reactor
    {
    private:
        struct file_io_event final
        {
            txl::file & file_;
            size_t num_read_;
            size_t num_total_;
            io_event on_data_;
            io_completer completer_;

            auto process(memory_pool & mem_pool) -> bool
            {
                auto to_read = std::min(static_cast<size_t>(4096), num_total_ - num_read_);
                auto mem_buf = mem_pool.allocate(to_read);

                auto bytes_read = file_.read(buffer_ref{mem_buf.data(), mem_buf.size()}.slice(0, to_read));
                
                // TODO: Notify and close on another thread
                on_data_(bytes_read);
                mem_buf.close();

                num_read_ += bytes_read.size();
                auto read_more = num_read_ < num_total_;
                if (not read_more)
                {
                    completer_.set_complete();
                }
                return read_more;
            }
        };

        class file_processor final
        {
        private:
            std::list<file_io_event> files_{};
            std::list<file_io_event>::iterator file_iter_;
            memory_pool & buf_in_;
        public:
            file_processor(memory_pool & mem_pool)
                : file_iter_(files_.begin())
                , buf_in_(mem_pool)
            {
            }

            auto add_file_reader(txl::file & file, size_t num_bytes, io_event on_data) -> io_task
            {
                auto & file_ev = files_.emplace_back(file, 0, num_bytes, on_data);
                return {file_ev.completer_};
            }
            
            auto process_one_file() -> void
            {
                if (file_iter_ != files_.end())
                {
                    if (not file_iter_->process(buf_in_))
                    {
                        file_iter_ = files_.erase(file_iter_);
                    }
                    else
                    {
                        ++file_iter_;
                    }
                    if (file_iter_ == files_.end())
                    {
                        file_iter_ = files_.begin();
                    }
                }
                else
                {
                    file_iter_ = files_.begin();
                }
            }
        };

        memory_pool buf_in_;
        event_poller poller_;
        std::thread io_thread_{};
        std::thread exec_thread_{};
        file_processor file_proc_;
        
        std::atomic_bool run_{true};


        auto io_thread_loop() -> void
        {
            //auto ev = txl::event_vector{30};
            while (run_.load())
            {
                file_proc_.process_one_file();
                /*auto num_available = poller_.poll(ev);
                if (num_available > 0)
                {
                    //for (
                }*/

            }
        }
    public:
        io_reactor()
            : buf_in_(16, 4096)
            , file_proc_(buf_in_)
        {
        }

        auto start() -> void
        {
            run_.store(true);
            io_thread_ = std::thread([this]() { io_thread_loop(); });
        }

        auto stop() -> void
        {
            run_.store(false);
        }

        auto join() -> void
        {
            io_thread_.join();
        }

        auto read_async(txl::file & file, size_t num_bytes, io_event on_data) -> io_task
        {
            return file_proc_.add_file_reader(file, num_bytes, on_data);
        }
    };
}
