#pragma once

#include <txl/buffer_ref.h>
#include <txl/io.h>
#include <txl/result.h>
#include <ostream>
#include <cstddef>

namespace txl
{
    class stream_writer final : public writer
    {
    private:
        std::ostream & os_;
    protected:
        // Returns buffer written
        auto write_impl(buffer_ref buf) -> result<size_t> override
        {
            auto before = os_.tellp();
            os_.write(reinterpret_cast<char const *>(buf.begin()), buf.size());
            return os_.tellp() - before;
        }
    public:
        stream_writer(std::ostream & os)
            : os_(os)
        {
        }
    };
}
