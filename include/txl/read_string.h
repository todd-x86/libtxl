#pragma once

#include <txl/copy.h>
#include <txl/io.h>
#include <txl/on_error.h>
#include <txl/stream_writer.h>
#include <txl/system_error.h>

#include <string>
#include <cstdlib>
#include <sstream>
#include <type_traits>

namespace txl
{
    template<class SizePolicy, class = std::enable_if_t<std::is_base_of_v<size_policy, SizePolicy>>>
    auto read_string(reader & rd, SizePolicy num_bytes, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> std::string
    {
        std::ostringstream ss{};
        stream_writer adapter{ss};
        copy(rd, adapter, num_bytes, on_err);
        return ss.str();
    }
    
    auto read_string(reader & rd, size_t num_bytes, on_error::callback<system_error> on_err = on_error::throw_on_error{}) -> std::string
    {
        return read_string(rd, exactly{num_bytes}, on_err);
    }
}
