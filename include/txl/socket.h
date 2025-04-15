#pragma once

#include <txl/buffer_ref.h>
#include <txl/file_base.h>
#include <txl/io.h>
#include <txl/result.h>
#include <txl/handle_error.h>
#include <txl/system_error.h>
#include <txl/socket_address.h>
#include <txl/socket_option.h>

#include <sys/socket.h>
#include <unistd.h>

namespace txl
{
    class socket : public file_base
                 , public reader
                 , public writer
    {
    protected:
        auto read_impl(buffer_ref buf) -> result<size_t> override
        {
            auto res = read(buf, io_flags::none);
            if (not res)
            {
                return res.error();
            }
            return res->size();
        }

        auto write_impl(buffer_ref buf) -> result<size_t> override
        {
            auto res = write(buf, io_flags::none);
            if (not res)
            {
                return res.error();
            }
            return res->size();
        }
        
        template<class T>
        auto get_option(int level, int optname) const -> result<T>
        {
            auto value_len = static_cast<socklen_t>(sizeof(T));
            auto value = T{};
            auto res = ::getsockopt(fd_, level, optname, static_cast<void *>(&value), &value_len);
            return handle_system_error(res, value);
        }

        template<class T>
        auto set_option(int level, int optname, T const & value) -> result<void>
        {
            auto res = ::setsockopt(fd_, level, optname, reinterpret_cast<void const *>(&value), static_cast<socklen_t>(sizeof(T)));
            return handle_system_error(res);
        }

        socket(int fd)
            : file_base(fd)
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
        socket(address_family af, socket_type t, int protocol = 0)
            : socket()
        {
            open(af, t, protocol).or_throw();
        }

        socket(socket const &) = delete;
        socket(socket && s) = default;

        auto open(address_family af, socket_type t, int protocol = 0) -> result<void>
        {
            if (is_open())
            {
                return get_system_error(EBUSY);
            }

            fd_ = ::socket(static_cast<int>(af), static_cast<int>(t), protocol);
            return handle_system_error(fd_);
        }
        
        using reader::read;

        auto read(buffer_ref buf, io_flags f) -> result<buffer_ref>
        {
            auto res = ::recv(fd_, buf.data(), buf.size(), static_cast<int>(f));
            if (auto err = handle_system_error(res); not err)
            {
                return err.error();
            }
            return buf.slice(0, res);
        }

        using writer::write;

        auto write(buffer_ref buf, io_flags f) -> result<buffer_ref>
        {
            auto res = ::send(fd_, buf.data(), buf.size(), static_cast<int>(f));
            if (auto err = handle_system_error(res); not err)
            {
                return err.error();
            }
            return buf.slice(0, res);
        }

        auto shutdown() -> result<void>
        {
            auto res = ::shutdown(fd_, SHUT_RDWR);
            return handle_system_error(res);
        }

        auto connect(socket_address const & sa) -> result<void>
        {
            auto res = ::connect(fd_, reinterpret_cast<::sockaddr const *>(sa.sockaddr()), sa.size());
            return handle_system_error(res);
        }

        auto accept(socket_address & out_addr, accept_flags flags = accept_flags::none) -> result<socket>
        {
            ::socklen_t out_sockaddr_len = sizeof(out_addr.addr_);
            auto res = ::accept4(fd_, reinterpret_cast<::sockaddr *>(&out_addr.addr_), &out_sockaddr_len, static_cast<int>(flags));
            if (auto err = handle_system_error(res); not err)
            {
                return err.error();
            }
            return socket(res);
        }
        
        auto accept(accept_flags flags = accept_flags::none) -> result<socket>
        {
            socket_address sa{};
            return accept(sa, flags);
        }

        auto bind(socket_address const & sa) -> result<void>
        {
            auto res = ::bind(fd_, reinterpret_cast<::sockaddr const *>(sa.sockaddr()), sa.size());
            return handle_system_error(res);
        }

        auto listen(int backlog) -> result<void>
        {
            auto res = ::listen(fd_, backlog);
            return handle_system_error(res);
        }

        auto get_remote_address() const -> result<socket_address>
        {
            socket_address sa{};
            ::socklen_t addr_sz = sa.size();
            auto res = ::getpeername(fd_, reinterpret_cast<::sockaddr *>(&sa.addr_), &addr_sz);
            return handle_system_error(res, sa);
        }

        auto get_local_address() const -> result<socket_address>
        {
            socket_address sa{};
            ::socklen_t addr_sz = sa.size();
            auto res = ::getsockname(fd_, reinterpret_cast<::sockaddr *>(&sa.addr_), &addr_sz);
            return handle_system_error(res, sa);
        }
        
        template<class T>
        auto get_option(socket_option::option<T> opt) const -> result<T>
        {
            return get_option<T>(opt.level, opt.opt_name);
        }

        template<class T>
        auto set_option(socket_option::option<T> opt, T const & value) -> result<void>
        {
            return set_option(opt.level, opt.opt_name, value);
        }
    };

    struct tcp_socket : socket
    {
        tcp_socket(bool open_socket = false)
            : socket()
        {
            if (open_socket)
            {
                open().or_throw();
            }
        }

        auto open() -> result<void>
        {
            return socket::open(socket::internet, socket::stream, 0);
        }
    };
}
