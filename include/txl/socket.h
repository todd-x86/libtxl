#pragma once

#include <txl/buffer_ref.h>

namespace txl
{
    class socket_result
    {
    private:
        int error_ = 0;
        size_t num_bytes_ = 0;
    public:
        socket_result() = default;
        socket_result(int err, size_t num_bytes)
            : error_(err)
            , num_bytes_(num_bytes)
        {
        }

        auto error_code() const -> int { return error_; }
        auto count() const -> size_t { return num_bytes_; }
    };

    class socket
    {
    private:
        int fd_;
    public:
        auto write(buffer_ref buf) -> socket_result
        {
            auto bytes_written = ::write(fd_, buf.data(), buf.size());
            return {errno, bytes_written};
        }
    };
}
