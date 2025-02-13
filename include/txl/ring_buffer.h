#pragma once

#include <txl/memory_map.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace txl
{
    enum class ring_buffer_flags : int
    {
        none = 0,
        preserve_values = 1,
        no_initialize = 1 << 1,
    };

    inline auto operator|(ring_buffer_flags x, ring_buffer_flags y) -> ring_buffer_flags
    {
        return static_cast<ring_buffer_flags>(
            static_cast<int>(x) | static_cast<int>(y)
        );
    }

    inline auto operator&(ring_buffer_flags x, ring_buffer_flags y) -> ring_buffer_flags
    {
        return static_cast<ring_buffer_flags>(
            static_cast<int>(x) & static_cast<int>(y)
        );
    }

    template<class Value>
    class ring_buffer
    {
    private:
        struct buffer_map final
        {
            size_t size_;
            char pad1_[64];
            size_t head_;
            char pad2_[64];
            size_t tail_;
            char pad3_[64];

            Value data_[0];
        };

        memory_map mmap_{};
        ring_buffer_flags flags_;

        auto slot(size_t index) const -> Value &
        {
            return mmap_.memory().to_alias<buffer_map>()->data_[index];
        }
        
        auto head() const -> size_t &
        {
            return mmap_.memory().to_alias<buffer_map>()->head_;
        }
        
        auto tail() const -> size_t &
        {
            return mmap_.memory().to_alias<buffer_map>()->tail_;
        }

        auto destruct_on_close() const -> bool
        {
            return (flags_ & ring_buffer_flags::preserve_values) != ring_buffer_flags::preserve_values;
        }
        
        auto is_no_initialize() const -> bool
        {
            return (flags_ & ring_buffer_flags::no_initialize) == ring_buffer_flags::no_initialize;
        }
    public:
        ring_buffer(size_t capacity, ring_buffer_flags flags = ring_buffer_flags::none, std::optional<int> fd = std::nullopt, std::optional<off_t> offset = std::nullopt)
            : flags_(flags)
        {
            auto mm_flags = memory_map::no_swap;
            if (not fd.has_value())
            {
                mm_flags = mm_flags | memory_map::anonymous;
            }
            mmap_.open(std::max(static_cast<size_t>(4096), (sizeof(Value) * capacity) + sizeof(buffer_map)), memory_map::read | memory_map::write, /* shared */ fd.has_value(), mm_flags, nullptr, fd, offset).or_throw();
            if (not is_no_initialize())
            {
                head() = 0;
                tail() = 0;
                mmap_.memory().to_alias<buffer_map>()->size_ = capacity;
            }
        }

        ~ring_buffer()
        {
            // Destruct all values
            if (destruct_on_close())
            {
                clear();
            }
            mmap_.close();
        }

        auto clear() -> void
        {
            while (head() != tail())
            {
                slot(head()).~Value();

                ++head();
                if (head() == capacity())
                {
                    head() = 0;
                }
            }
        }

        size_t size() const
        {
            if (tail() >= head())
            {
                return tail() - head();
            }
            else
            {
                return ((head() + capacity() - 1) - tail());
            }
        }

        auto capacity() const -> size_t { return mmap_.memory().to_alias<buffer_map>()->size_; }

        auto read() -> std::optional<Value>
        {
            if (head() == tail())
            {
                return {};
            }

            auto res = std::make_optional(std::move(slot(head())));

            ++head();
            if (head() == capacity())
            {
                head() = 0;
            }

            return res;
        }

        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            if (head() == ((tail() + 1) % capacity()))
            {
                // Destruct old value
                slot(head()).~Value();
            }

            auto & s = slot(tail());
            new(&s) Value(std::forward<Args>(args)...);
            ++tail();
            if (tail() == capacity())
            {
                tail() = 0;
            }

            // Move head forward
            if (head() == tail())
            {
                ++head();
                if (head() == capacity())
                {
                    head() = 0;
                }
            }
        }
    };
}
