#pragma once

#include <txl/io.h>
#include <txl/patterns.h>
#include <txl/system_error.h>
#include <txl/result.h>
#include <txl/handle_error.h>

#include <sys/sendfile.h>
#include <unistd.h>
#include <algorithm>
#include <optional>

namespace txl
{
    class file_base
    {
    protected:
        int fd_ = -1;

        file_base(int fd)
            : fd_(fd)
        {
        }
    public:
        file_base() = default;

        file_base(file_base const &) = delete;

        file_base(file_base && f)
            : file_base(f.fd_)
        {
            f.fd_ = -1;
        }

        virtual ~file_base()
        {
            if (is_open())
            {
                close();
            }
        }

        auto operator=(file_base const &) -> file_base & = delete;
        auto operator=(file_base && f) -> file_base &
        {
            if (this != &f)
            {
                if (is_open())
                {
                    close();
                }
                std::swap(f.fd_, fd_);
            }
            return *this;
        }

        auto fd() const -> int { return fd_; }

        auto is_open() const -> bool { return fd_ != -1; }

        auto close() -> result<void>
        {
            auto result = handle_system_error(::close(fd_));
            // Signify it's closed even if close() returns an error.
            // close() only returns an error if it's already been closed
            // or was an invalid FD.
            fd_ = -1;
            return result;
        }
    };    
}
