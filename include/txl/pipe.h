#pragma once

#include <txl/buffer_ref.h>
#include <txl/file_base.h>
#include <txl/handle_error.h>
#include <txl/io.h>
#include <txl/result.h>
#include <txl/system_error.h>

#include <cerrno>
#include <cstdlib>

namespace txl
{
    class pipe final : public file_base
                     , public reader
                     , public writer
    {
        friend class pipe_connector;
    private:
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
    protected:
        pipe(int fd)
            : file_base(fd)
        {
        }
    public:
        pipe() = default;
        pipe(pipe const &) = delete;
        pipe(pipe &&) = default;

        auto operator=(pipe const &) -> pipe & = delete;
        auto operator=(pipe &&) -> pipe & = default;
        
        using reader::read;
        using writer::write;
    };

    class pipe_connector final
    {
    private:
        pipe input_{};
        pipe output_{};
    public:
        auto open() -> result<void>
        {
            if (input_.is_open() or output_.is_open())
            {
                return get_system_error(EBUSY);
            }

            int fds[2];
            // TODO: support flags
            auto res = handle_system_error(::pipe2(fds, 0));
            if (res)
            {
                input_ = pipe{fds[0]};
                output_ = pipe{fds[1]};
            }
            return res;
        }

        auto input() -> pipe & { return input_; }
        auto output() -> pipe & { return output_; }
    };
}
