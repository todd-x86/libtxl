#pragma once

#include <txl/buffer_ref.h>
#include <txl/file_base.h>
#include <txl/handle_error.h>
#include <txl/io.h>
#include <txl/on_error.h>
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
        auto read_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t override
        {
            auto bytes_read = ::read(fd_, buf.data(), buf.size());
            if (handle_system_error(bytes_read, on_err))
            {
                return bytes_read;
            }
            return 0;
        }

        auto write_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t override
        {
            auto bytes_written = ::write(fd_, buf.data(), buf.size());
            if (handle_system_error(bytes_written, on_err))
            {
                return bytes_written;
            }
            return 0;
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
        auto open(on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> void
        {
            if (input_.is_open() or output_.is_open())
            {
                on_err(EBUSY);
                return;
            }

            int fds[2];
            // TODO: support flags
            auto res = ::pipe2(fds, 0);
            if (handle_system_error(res, on_err))
            {
                input_ = pipe{fds[0]};
                output_ = pipe{fds[1]};
            }
        }

        auto input() -> pipe & { return input_; }
        auto output() -> pipe & { return output_; }
    };
}
