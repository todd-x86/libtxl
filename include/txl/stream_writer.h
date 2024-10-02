#pragma once

#include <txl/io.h>
#include <ostream>

namespace txl
{
    class stream_writer final : public writer
    {
    private:
        std::ostream & os_;
    protected:
        // Returns buffer written
        auto write_impl(buffer_ref buf, on_error::callback<system_error> on_err) -> size_t override
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
