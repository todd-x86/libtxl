#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/on_error.h>
#include <txl/handle_error.h>
#include <txl/system_error.h>
#include <txl/socket_address.h>

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>

namespace txl
{
    class socket : public reader
                 , public writer
    {
    private:
        int fd_ = -1;
    protected:
        auto read_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t override
        {
            return read(buf, io_flags::none, on_err).size();
        }

        auto write_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t override
        {
            return write(buf, io_flags::none, on_err).size();
        }

        socket(int fd)
            : fd_(fd)
        {
        }
    public:
        enum class io_flags : int
        {
            none = 0,
            non_block = MSG_DONTWAIT,
            error_queue = MSG_ERRQUEUE,
        };

        enum class accept_flags : int
        {
            none = 0,
            non_block = SOCK_NONBLOCK,
            close_on_exec = SOCK_CLOEXEC,
        };

        enum address_family : int
        {
            internet = AF_INET,
            unix_socket = AF_UNIX,
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

        auto fd() const -> int
        {
            return fd_;
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

        auto read(buffer_ref buf, io_flags f, on_error::callback<system_error> on_err) -> buffer_ref
        {
            auto res = ::recv(fd_, buf.data(), buf.size(), static_cast<int>(f));
            if (handle_system_error(res, on_err))
            {
                return buf.slice(0, res);
            }
            return {};
        }

        using writer::write;

        auto write(buffer_ref buf, io_flags f, on_error::callback<system_error> on_err) -> buffer_ref
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
            return handle_system_error(res, on_err);
        }

        auto connect(socket_address const & sa, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> bool
        {
            auto res = ::connect(fd_, reinterpret_cast<::sockaddr const *>(sa.sockaddr()), sa.size());
            return handle_system_error(res, on_err);
        }

        auto accept(socket_address & out_addr, accept_flags flags = accept_flags::none, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> socket
        {
            ::socklen_t out_sockaddr_len = sizeof(out_addr.addr_);
            auto res = ::accept4(fd_, reinterpret_cast<::sockaddr *>(&out_addr.addr_), &out_sockaddr_len, static_cast<int>(flags));
            if (handle_system_error(res, on_err))
            {
                return socket(res);
            }
            return {};
        }
        
        auto accept(accept_flags flags = accept_flags::none, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> socket
        {
            socket_address sa{};
            return accept(sa, flags, on_err);
        }

        auto bind(socket_address const & sa, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> bool
        {
            auto res = ::bind(fd_, reinterpret_cast<::sockaddr const *>(sa.sockaddr()), sa.size());
            return handle_system_error(res, on_err);
        }

        auto listen(int backlog, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> bool
        {
            auto res = ::listen(fd_, backlog);
            return handle_system_error(res, on_err);
        }

        auto get_remote_address(on_error::callback<system_error> on_err = on_error::throw_on_error{}) const -> socket_address
        {
            socket_address sa{};
            ::socklen_t addr_sz = sa.size();
            auto res = ::getpeername(fd_, reinterpret_cast<::sockaddr *>(&sa.addr_), &addr_sz);
            handle_system_error(res, on_err);
            return sa;
        }

        auto get_local_address(on_error::callback<system_error> on_err = on_error::throw_on_error{}) const -> socket_address
        {
            socket_address sa{};
            ::socklen_t addr_sz = sa.size();
            auto res = ::getsockname(fd_, reinterpret_cast<::sockaddr *>(&sa.addr_), &addr_sz);
            handle_system_error(res, on_err);
            return sa;
        }

        /*template<class Value>
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
        }*/
    };
}
