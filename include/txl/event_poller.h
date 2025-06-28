#pragma once

#include <txl/file_base.h>
#include <txl/handle_error.h>
#include <txl/result.h>
#include <txl/system_error.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <ostream>
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
    
    inline auto operator|(event_type x, event_type y) -> event_type
    {
        auto xy = static_cast<uint32_t>(x) | static_cast<uint32_t>(y);
        return static_cast<event_type>(xy);
    }

    inline auto operator&(event_type x, event_type y) -> event_type
    {
        auto xy = static_cast<uint32_t>(x) & static_cast<uint32_t>(y);
        return static_cast<event_type>(xy);
    }

    inline auto operator<<(std::ostream & os, event_type t) -> std::ostream &
    {
        auto comma = false;
        os << '{';
        if (static_cast<uint32_t>(t & event_type::in) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "in";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::out) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "out";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::read_hangup) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "read_hangup";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::priority) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "priority";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::error) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "error";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::hangup) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "hangup";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::edge) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "edge";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::one_shot) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "one_shot";
            comma = true;
        }
        if (static_cast<uint32_t>(t & event_type::wakeup) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "wakeup";
            comma = true;
        }
#ifdef EPOLLEXCLUSIVE
        if (static_cast<uint32_t>(t & event_type::exclusive) != 0)
        {
            if (comma)
            {
                os << ',';
            }
            os << "exclusive";
            comma = true;
        }
#endif
        os << '}';
        return os;
    }

    class event_tag final
    {
        friend struct event_poller;
    private:
        ::epoll_data data_{};
    public:
        static auto from_fd(int v) -> event_tag
        {
            event_tag tag{};
            tag.fd(v);
            return tag;
        }

        auto fd() const -> int { return data_.fd; }
        auto fd(int v) -> void { data_.fd = v; }
        
        static auto from_ptr(void * v) -> event_tag
        {
            event_tag tag{};
            tag.ptr(v);
            return tag;
        }
        
        auto ptr() const -> void * { return data_.ptr; }
        auto ptr(void * v) -> void { data_.ptr = v; }
        
        static auto from_u32(uint32_t v) -> event_tag
        {
            event_tag tag{};
            tag.u32(v);
            return tag;
        }
        
        auto u32() const -> uint32_t { return data_.u32; }
        auto u32(uint32_t v) -> void { data_.u32 = v; }
        
        static auto from_u64(uint64_t v) -> event_tag
        {
            event_tag tag{};
            tag.u64(v);
            return tag;
        }
        
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
        auto has_one_of(event_type t) const -> bool { return static_cast<uint32_t>(events() & t) != 0; }
        auto has_all(event_type t) const -> bool { return (events() & t) == t; }
        auto fd() const -> int { return data.fd; }
    };

    template<size_t S>
    class event_array final : public event_buffer
    {
    private:
        using container_type = std::array<event_data, S>;
        container_type data_;
    public:
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

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

        auto open() -> result<void>
        {
            if (is_open())
            {
                return {};
            }

            fd_ = ::epoll_create1(0);
            return handle_system_error(fd_);
        }

        auto add(int fd, event_type flags) -> result<void>
        {
            auto t = event_tag{};
            t.fd(fd);
            
            return add(fd, flags, t);
        }

        auto add(int fd, event_type flags, event_tag tag) -> result<void>
        {
            auto event_details = ::epoll_event{};
            event_details.events = static_cast<int>(flags);
            event_details.data = tag.data_;
            
            auto res = ::epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &event_details);
            return handle_system_error(res);
        }
        
        auto modify(int fd, event_type flags) -> result<void>
        {
            auto t = event_tag{};
            t.fd(fd);
            
            return modify(fd, flags, t);
        }

        auto modify(int fd, event_type flags, event_tag tag) -> result<void>
        {
            auto event_details = ::epoll_event{};
            event_details.events = static_cast<int>(flags);
            event_details.data = tag.data_;
            auto res = ::epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &event_details);
            return handle_system_error(res);
        }
        
        auto remove(int fd) -> result<void>
        {
            auto res = ::epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr);
            return handle_system_error(res);
        }

        auto poll(event_buffer & buf, std::optional<std::chrono::milliseconds> timeout = std::nullopt) -> result<int>
        {
            // TODO: add sigset mask support
            auto res = ::epoll_pwait(fd_, buf.epoll_buffer(), buf.size(), timeout.value_or(std::chrono::milliseconds{-1}).count(), nullptr);
            return handle_system_error(res, res);
        }
    };
}
