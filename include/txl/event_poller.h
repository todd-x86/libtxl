#pragma once

#include <txl/file_base.h>
#include <txl/handle_error.h>
#include <txl/on_error.h>
#include <txl/system_error.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace txl
{
    enum class event_type : uint32_t
    {
        in = EPOLLIN,
        out = EPOLLOUT,
        read_hangup = EPOLLRDHUP,
        priority = EPOLLPRI,
        error = EPOLLERR,
        hangup = EPOLLHUP,
        edge = EPOLLET,
        one_shot = EPOLLONESHOT,
        wakeup = EPOLLWAKEUP,
#ifdef EPOLLEXCLUSIVE
        exclusive = EPOLLEXCLUSIVE,
#endif
    };

    class event_tag final
    {
        friend class event_poller;
    private:
        ::epoll_data data_{};
    public:
        auto fd() const -> int { return data_.fd; }
        auto fd(int v) -> void { data_.fd = v; }
        
        auto ptr() const -> void * { return data_.ptr; }
        auto ptr(void * v) -> void { data_.ptr = v; }
        
        auto u32() const -> uint32_t { return data_.u32; }
        auto u32(uint32_t v) -> void { data_.u32 = v; }
        
        auto u64() const -> uint64_t { return data_.u64; }
        auto u64(uint64_t v) -> void { data_.u64 = v; }
    };
    
    struct event_buffer
    {
        virtual auto epoll_buffer() -> ::epoll_event * = 0;
        virtual auto size() const -> size_t = 0;
    };

    struct event_view final
    {
    private:
        ::epoll_event const & event_;
    public:
        event_view(::epoll_event const & e)
            : event_(e)
        {
        }

        auto events() const -> event_type { return static_cast<event_type>(event_.events); }
        auto fd() const -> int { return event_.data.fd; }
    };

    template<size_t S>
    class event_array final : public event_buffer
    {
    private:
        std::array<::epoll_event, S> data_;
    public:
        auto operator[](size_t index) const -> event_view
        {
            return event_view{data_[index]};
        }

        auto epoll_buffer() -> ::epoll_event * override
        {
            return &data_[0];
        }

        auto size() const -> size_t override
        {
            return S;
        }
    };

    class event_vector final : public event_buffer
    {
    private:
        std::vector<::epoll_event> data_;
    public:
        event_vector() = default;
        event_vector(size_t size)
        {
            data_.resize(size);
        }

        auto resize(size_t size) -> void
        {
            data_.resize(size);
        }

        auto epoll_buffer() -> ::epoll_event * override
        {
            return &data_.front();
        }

        auto size() const -> size_t override
        {
            return data_.size();
        }
    };

    struct event_poller final : file_base
    {
        event_poller() = default;
        event_poller(event_poller const &) = delete;
        event_poller(event_poller && p) = default;

        auto open(on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            if (is_open())
            {
                return;
            }

            fd_ = ::epoll_create1(0);
            handle_system_error(fd_, on_err);
        }

        auto add(int fd, event_type flags, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto t = event_tag{};
            t.fd(fd);
            
            add(fd, flags, t, on_err);
        }

        auto add(int fd, event_type flags, event_tag tag, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto event_details = ::epoll_event{};
            event_details.events = static_cast<int>(flags);
            event_details.data = tag.data_;
            
            auto res = ::epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &event_details);
            handle_system_error(res, on_err);
        }
        
        auto modify(int fd, event_type flags, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto t = event_tag{};
            t.fd(fd);
            
            modify(fd, flags, t, on_err);
        }

        auto modify(int fd, event_type flags, event_tag tag, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto event_details = ::epoll_event{};
            event_details.events = static_cast<int>(flags);
            event_details.data = tag.data_;
            auto res = ::epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &event_details);
            handle_system_error(res, on_err);
        }
        
        auto remove(int fd, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto res = ::epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr);
            handle_system_error(res, on_err);
        }

        auto poll(event_buffer & buf, std::optional<std::chrono::milliseconds> timeout = std::nullopt, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> int
        {
            // TODO: add sigset mask support
            auto res = ::epoll_pwait(fd_, buf.epoll_buffer(), buf.size(), timeout.value_or(std::chrono::milliseconds{-1}).count(), nullptr);
            handle_system_error(res, on_err);
            return res;
        }
    };
}
