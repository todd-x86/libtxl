#pragma once

#include <filesystem>

namespace txl
{
    /**
     * Reads the current process's executable path.
     * Follows the "/proc/self/exe" symlink.
     * 
     * \return resolved path of the running executable
     */
    inline auto get_application_path() -> std::filesystem::path
    {
        return std::filesystem::read_symlink("/proc/self/exe").parent_path();
    }
}
