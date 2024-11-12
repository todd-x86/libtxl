#pragma once

#include <filesystem>

namespace txl
{
    inline auto get_application_path() -> std::filesystem::path
    {
        return std::filesystem::read_symlink("/proc/self/exe").parent_path();
    }
}
