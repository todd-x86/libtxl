#pragma once

#include <txl/handle_error.h>
#include <txl/result.h>

#include <sys/statvfs.h>

namespace txl
{
    struct vfs_info final : ::statvfs
    {
        auto num_free_blocks() const -> size_t
        {
            return f_bfree;
        }
    };

    inline auto get_vfs_info(int fd) -> result<vfs_info>
    {
        vfs_info info{};
        auto res = ::fstatvfs(fd, &info);
        return handle_system_error(res, std::move(info));
    }
}
