#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/on_error.h>
#include <txl/handle_error.h>
#include <txl/system_error.h>

#include <algorithm>

namespace txl
{
    class socket : public reader
                 , public writer
    {
    private:
        int fd_ = -1;
    protected:
        auto read_impl(buffer_ref buf, on_error::callback<system_error> on_err) override -> buffer_ref
        {
            auto res = ::recv(fd_, buf.data(), buf.size(), 0);
            if (handle_system_error(res, on_err))
            {
                return buf.slice(0, res);
            }
            return {};
        }

        auto write_impl(buffer_ref buf, on_error::callback<system_error> on_err) override -> buffer_ref
        {
        }
    public:
        enum flags : int
        {
            none = 0,
            non_block = MSG_DONTWAIT,
            error_queue = MSG_ERRQUEUE,
        };

        socket() = default;
        socket(
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

        auto close(on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto res = ::close(fd_);
            if (handle_system_error(res, on_err))
            {
                fd_ = -1;
            }
        }
    };
}
