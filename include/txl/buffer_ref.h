#pragma once

#include <algorithm>
#include <iterator>

namespace txl
{
    class buffer_ref final
    {
    private:
        void * buffer_ = nullptr;
        size_t length_ = 0;
    public:
        buffer_ref() = default;
        buffer_ref(void * buffer, size_t length)
            : buffer_(buffer)
            , length_(length)
        {
        }

        auto operator==(buffer_ref const & other) const -> bool
        {
            return data() == other.data() and size() == other.size();
        }
        auto operator!=(buffer_ref const & other) const -> bool
        {
            return !(*this == other);
        }

        auto slice(size_t begin, size_t end) -> buffer_ref
        {
            auto buf = reinterpret_cast<char *>(buffer_);
            if (begin >= length_ || end <= begin)
            {
                return {};
            }
            end = std::min(length_ - begin, end - begin);
            return buffer_ref{reinterpret_cast<void *>(std::next(buf, begin)), end};
        }

        auto slice(size_t begin) -> buffer_ref
        {
            return slice(begin, length_);
        }

        auto data() -> void * { return buffer_; }
        auto data() const -> void const * { return buffer_; }
        auto size() const -> size_t { return length_; }
        auto begin() -> void * { return buffer_; }
        auto end() -> void * { return reinterpret_cast<void *>(std::next(reinterpret_cast<char *>(buffer_), length_)); }
        auto begin() const -> void const * { return buffer_; }
        auto end() const -> void const * { return reinterpret_cast<void const *>(std::next(reinterpret_cast<char const *>(buffer_), length_)); }
    };
}
