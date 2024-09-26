#pragma once

#include <txl/io.h>
#include <txl/buffer_ref.h>
#include <txl/result.h>
#include <txl/system_error.h>
#include <txl/on_error.h>
#include <txl/handle_error.h>

#include <string_view>
#include <algorithm>

#include <fcntl.h>
#include <unistd.h>

namespace txl
{
    class file : public reader
               , public writer
    {
    private:
        int fd_ = -1;

        static auto get_file_mode(std::string_view s, on_error::callback<system_error> on_err) -> int
        {
            // r = O_RDONLY
            // w = O_WRONLY | O_CREAT | O_TRUNC
            // a = O_WRONLY | O_CREAT | O_APPEND
            // r+ = O_RDWR
            // w+ = O_RDWR | O_CREAT | O_TRUNC
            // a+ = O_RDWR | O_CREAT | O_APPEND
            if (s.length() == 1)
            {
                switch (s[0])
                {
                    case 'r': return O_RDONLY;
                    case 'w': return O_WRONLY | O_CREAT | O_TRUNC;
                    case 'a': return O_WRONLY | O_CREAT | O_APPEND;
                }
            }
            else if (s.length() == 2 && s[1] == '+')
            {
                switch (s[0])
                {
                    case 'r': return O_RDWR;
                    case 'w': return O_RDWR | O_CREAT | O_TRUNC;
                    case 'a': return O_RDWR | O_CREAT | O_APPEND;
                }
            }
            
            on_err(EINVAL);
            return 0;
        }

        auto read_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t
        {
            auto bytes_read = ::read(fd_, buf.data(), buf.size());
            if (handle_system_error(bytes_read, on_err))
            {
                return bytes_read;
            }
            return 0;
        }

        auto write_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t
        {
            auto bytes_written = ::write(fd_, buf.data(), buf.size());
            if (handle_system_error(bytes_written, on_err))
            {
                return bytes_written;
            }
            return 0;
        }
    public:
        enum seek_type : int
        {
            seek_set = SEEK_SET,
            seek_current = SEEK_CUR,
            seek_end = SEEK_END,
        };

        file() = default;
        file(std::string_view filename, std::string_view mode, on_error::callback<system_error> on_open_err = on_error::throw_on_error{})
            : file()
        {
            open(filename, mode, on_open_err);
        }

        file(file const &) = delete;
        file(file && f)
            : file()
        {
            std::swap(f.fd_, fd_);
        }

        virtual ~file()
        {
            close(on_error::ignore{});
        }

        auto open(std::string_view filename, std::string_view mode, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            static constexpr const int DEFAULT_FILE_PERMS = S_IRUSR | S_IWUSR | S_IRGRP;

            if (is_open())
            {
                // Already in use
                on_err({EACCES});
                return;
            }

            auto file_mode = get_file_mode(mode, on_err);

            fd_ = ::open(filename.data(), file_mode, DEFAULT_FILE_PERMS);
            handle_system_error(fd_, on_err);
        }

        auto is_open() const -> bool { return fd_ != -1; }

        auto close(on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            auto res = ::close(fd_);
            if (handle_system_error(res, on_err))
            {
                fd_ = -1;
            }
        }

        auto seek(off_t offset, seek_type st = seek_set, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> off_t
        {
            auto res = ::lseek(fd_, offset, static_cast<int>(st));
            handle_system_error(res, on_err);
            return res;
        }
    };
}
