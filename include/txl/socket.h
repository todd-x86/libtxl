#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/on_error.h>
#include <txl/handle_error.h>
#include <txl/system_error.h>

#include <sys/socket.h>

#include <algorithm>

namespace txl
{
    class socket : public reader
                 , public writer
    {
    private:
        int fd_ = -1;
    protected:
        auto read_impl(buffer_ref buf, on_error::callback<system_error> on_err) override -> size_t
        {
            return read(buf, none, on_err).size();
        }

        auto write_impl(buffer_ref buf, on_error::callback<system_error> on_err) override -> size_t
        {
            return write(buf, none, on_err).size();
        }
    public:
        enum flags : int
        {
            none = 0,
            non_block = MSG_DONTWAIT,
            error_queue = MSG_ERRQUEUE,
        };

        enum address_family : int
        {
            internet = AF_INET,
            unix = AF_UNIX,
            local = AF_LOCAL,
        };

        enum socket_type : int
        {
            raw = SOCK_RAW,
            stream = SOCK_STREAM,
            datagram = SOCK_DGRAM,
            sequenced = SOCK_SEQPACKET,
        };

        socket() = default;
        socket(address_family af, socket_type t, int protocol = 0, on_error::callback<system_error> on_err = on_error::throw_on_error{})
            : socket()
        {
            open(af, t, protocol, on_err);
        }

        socket(socket const &) = delete;
        socket(socket && s)
            : socket()
        {
            std::swap(s.fd_, fd_);
        }

        virtual ~socket()
        {
            close(on_error::ignore{});
        }

        auto is_open() const -> bool
        {
            return fd_ != -1;
        }

        auto open(address_family af, socket_type t, int protocol = 0, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> bool
        {
            fd_ = ::socket(static_cast<int>(af), static_cast<int>(t), protocol);
            return handle_system_error(fd_, on_err);
        }
        
        using reader::read;

        auto read(buffer_ref buf, flags f, on_error::callback<system_error> on_err) override -> buffer_ref
        {
            auto res = ::recv(fd_, buf.data(), buf.size(), static_cast<int>(f));
            if (handle_system_error(res, on_err))
            {
                return buf.slice(0, res);
            }
            return {};
        }

        using writer::write;

        auto write(buffer_ref buf, flags f, on_error::callback<system_error> on_err) override -> buffer_ref
        {
            auto res = ::send(fd_, buf.data(), buf.size(), static_cast<int>(f));
            if (handle_system_error(res, on_err))
            {
                return buf.slice(0, res);
            }
            return {};
        }

        auto close(on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto res = ::close(fd_);
            if (handle_system_error(res, on_err))
            {
                fd_ = -1;
            }
        }
        
        auto shutdown(on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> bool
        {
            auto res = ::shutdown(fd_, SHUT_RDWR);
            return handle_system_error_code(res, on_err);
        }

        template<class SockAddr>
        bool connect(SockAddr * addr, size_t addr_size, error_code & err)
        {
            auto res = ::connect(fd(), reinterpret_cast<::sockaddr const *>(addr), addr_size);
            return handle_error_code(res, err);
        }

        auto accept(::sockaddr * out_sockaddr, ::socklen_t * out_sockaddr_len, int flags, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> socket
        {
            auto res = ::accept4(fd(), reinterpret_cast<::sockaddr *>(out_sockaddr), out_sockaddr_len, flags);
            handle_error_code(res, err);
            return accepted_socket(res);
        }

        template<class SockAddr>
        auto bind(SockAddr * addr) -> bool
        {
            static_assert(sizeof(SockAddr) >= sizeof(sockaddr), "SockAddr must be at least sizeof(struct sockaddr)");
            auto res = ::bind(fd(), reinterpret_cast<sockaddr const *>(addr), sizeof(SockAddr));
            return handle_error_code(res, err);
        }


        auto listen(int backlog, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> bool
        {
            return handle_error_code(::listen(fd(), backlog), err);
        }

        template<class Value>
        bool getsockopt(int level, int optname, Value * value, error_code & err) const
        {
            auto value_len = static_cast<socklen_t>(sizeof(Value));
            auto res = ::getsockopt(fd(), level, optname, reinterpret_cast<void *>(value), &value_len);
            return handle_error_code(res, err);
        }

        template<class Value>
        bool setsockopt(int level, int optname, Value const * value, error_code & err)
        {
            auto res = ::setsockopt(fd(), level, optname, reinterpret_cast<void const *>(value), static_cast<socklen_t>(sizeof(Value)));
            return handle_error_code(res, err);
        }
    };
}
