#include <txl/expect.h>

namespace txl
{
    thread_local std::function<void(expect const &, std::chrono::steady_clock::time_point const &)> expect::on_at_most_{};
}
