#pragma once

#include <txl/result.h>
#include <txl/file_base.h>
#include <txl/io.h>
#include <txl/buffer_ref.h>
#include <txl/system_error.h>
#include <txl/result.h>
#include <txl/handle_error.h>

#include <string_view>

#include <fcntl.h>
#include <unistd.h>

namespace txl
{
    class file : public file_base
               , public reader
               , public writer
    {
    private:
        static auto get_file_mode(std::string_view s) -> result<int>
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
            
            return get_system_error(EINVAL);
        }

        auto read_impl(buffer_ref buf) -> result<size_t> override
        {
            auto bytes_read = ::read(fd_, buf.data(), buf.size());
            return handle_system_error(bytes_read, static_cast<size_t>(bytes_read));
        }

        auto write_impl(buffer_ref buf) -> result<size_t> override
        {
            auto bytes_written = ::write(fd_, buf.data(), buf.size());
            return handle_system_error(bytes_written, static_cast<size_t>(bytes_written));
        }
    public:
        enum seek_type : int
        {
            seek_set = SEEK_SET,
            seek_current = SEEK_CUR,
            seek_end = SEEK_END,
        };

        file() = default;
        file(std::string_view filename, std::string_view mode)
            : file()
        {
            open(filename, mode).or_throw();
        }

        file(file const &) = delete;
        file(file && f) = default;

        auto open(std::string_view filename, std::string_view mode) -> result<void>
        {
            static constexpr const int DEFAULT_FILE_PERMS = S_IRUSR | S_IWUSR | S_IRGRP;

            if (is_open())
            {
                // Already in use
                return get_system_error(EACCES);
            }

            auto file_mode = get_file_mode(mode);
            if (!file_mode)
            {
                return file_mode.error();
            }

            fd_ = ::open(filename.data(), *file_mode, DEFAULT_FILE_PERMS);
            return handle_system_error(fd_);
        }

        using reader::read;

        auto read(off_t offset, buffer_ref buf) -> result<buffer_ref>
        {
            auto bytes_read = ::pread(fd_, buf.data(), buf.size(), offset);
            auto res = handle_system_error(bytes_read, static_cast<size_t>(bytes_read));
            if (not res)
            {
                return res.error();
            }
            return buf.slice(0, *res);
        }

        using writer::write;

        auto write(off_t offset, buffer_ref buf) -> result<buffer_ref>
        {
            auto bytes_written = ::pwrite(fd_, buf.data(), buf.size(), offset);
            auto res = handle_system_error(bytes_written, static_cast<size_t>(bytes_written));
            if (not res)
            {
                return res.error();
            }
            return buf.slice(0, *res);
        }

        auto tell() -> result<off_t>
        {
            return seek(0, seek_current);
        }

        auto seek(off_t offset, seek_type st = seek_set) -> result<off_t>
        {
            auto res = ::lseek(fd_, offset, static_cast<int>(st));
            return handle_system_error(res, res);
        }

        auto seekable_size() -> result<size_t>
        {
            // Record position and seek to end
            auto pos = tell();
            if (!pos)
            {
                return pos.error();
            }

            auto end_pos = seek(0, seek_end);
            if (!end_pos)
            {
                return end_pos.error();
            }

            // Reset back
            auto res = seek(*pos, seek_set);
            if (!res)
            {
                return res.error();
            }

            return static_cast<size_t>(*end_pos);
        }

        auto truncate(size_t size) -> result<void>
        {
            auto res = ::ftruncate(fd_, size);
            return handle_system_error(res);
        }
    };
}
