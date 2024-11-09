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

    struct event_data : private ::epoll_event
    {
        auto events() const -> event_type { return static_cast<event_type>(::epoll_event::events); }
        auto fd() const -> int { return data.fd; }
    };

    template<size_t S>
    class event_array final : public event_buffer
    {
    private:
        using container_type = std::array<event_data, S>;
        container_type data_;
    public:
        using iterator = container_type::iterator;
        using const_iterator = container_type::const_iterator;

        auto begin() -> iterator { return data_.begin(); }
        auto end() -> iterator { return data_.end(); }
        auto begin() const -> const_iterator { return data_.begin(); }
        auto end() const -> const_iterator { return data_.end(); }
        auto cbegin() const -> const_iterator { return data_.cbegin(); }
        auto cend() const -> const_iterator { return data_.cend(); }

        auto operator[](size_t index) const -> event_data const &
        {
            return data_[index];
        }

        auto epoll_buffer() -> ::epoll_event * override
        {
            return reinterpret_cast<::epoll_event *>(&data_[0]);
        }

        auto size() const -> size_t override
        {
            return S;
        }
    };

    class event_vector final : public event_buffer
    {
    private:
        using container_type = std::vector<event_data>;
        container_type data_;
    public:
        using iterator = container_type::iterator;
        using const_iterator = container_type::const_iterator;

        event_vector() = default;
        event_vector(size_t size)
        {
            data_.resize(size);
        }
        
        auto begin() -> iterator { return data_.begin(); }
        auto end() -> iterator { return data_.end(); }
        auto begin() const -> const_iterator { return data_.begin(); }
        auto end() const -> const_iterator { return data_.end(); }
        auto cbegin() const -> const_iterator { return data_.cbegin(); }
        auto cend() const -> const_iterator { return data_.cend(); }
        
        auto operator[](size_t index) const -> event_data const &
        {
            return data_[index];
        }

        auto resize(size_t size) -> void
        {
            data_.resize(size);
        }

        auto epoll_buffer() -> ::epoll_event * override
        {
            return reinterpret_cast<::epoll_event *>(&data_.front());
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
