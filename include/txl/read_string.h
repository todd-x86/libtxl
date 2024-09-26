#pragma once

#include <txl/copy.h>
#include <txl/io.h>
#include <txl/on_error.h>
#include <txl/system_error.h>

#include <string>
#include <cstdlib>
#include <sstream>

namespace txl
{
    auto read_string(reader & rd, size_t num_bytes, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> std::string
    {
        std::ostringstream ss{};
        copy(rd, ss, num_bytes, on_err);
        return ss.str();
    }
}
