#pragma once

#include <algorithm>
#include <string_view>
#include <iterator>
#include <array>

namespace txl
{
    class buffer_ref final
    {
    private:
        void * buffer_ = nullptr;
        size_t length_ = 0;
    public:
        buffer_ref() = default;

        template<class Char>
        buffer_ref(std::basic_string_view<Char> s)
            : buffer_ref(reinterpret_cast<void *>(const_cast<char *>(s.data())), s.size())
        {
        }

        template<class Char, size_t Size>
        buffer_ref(std::array<Char, Size> & buf)
            : buffer_ref(buf.data(), Size)
        {
        }

        buffer_ref(char const * s)
            : buffer_ref(std::string_view{s})
        {
        }

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
            if (begin >= length_ or end <= begin)
            {
                return {};
            }
            end = std::min(length_ - begin, end - begin);
            return {reinterpret_cast<void *>(std::next(buf, begin)), end};
        }

        auto slice(size_t begin) -> buffer_ref
        {
            return slice(begin, length_);
        }

        auto to_string_view() const -> std::string_view { return std::string_view{reinterpret_cast<char const *>(data()), size()}; }

        auto empty() const -> bool { return length_ == 0; }
        auto data() -> void * { return buffer_; }
        auto data() const -> void const * { return buffer_; }
        auto size() const -> size_t { return length_; }
        auto begin() -> void * { return buffer_; }
        auto end() -> void * { return reinterpret_cast<void *>(std::next(reinterpret_cast<char *>(buffer_), length_)); }
        auto begin() const -> void const * { return buffer_; }
        auto end() const -> void const * { return reinterpret_cast<void const *>(std::next(reinterpret_cast<char const *>(buffer_), length_)); }
    };
}
