#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace txl
{
    template<class Value>
    class ring_buffer
    {
    private:
        struct slot
        {
            uint8_t data_[sizeof(Value)];

            Value & val() { return *reinterpret_cast<Value *>(data_); }
            Value const & val() const { return *reinterpret_cast<Value const *>(data_); }
        };
        std::vector<slot> slots_;
        size_t head_ = 0;
        size_t tail_ = 0;
    public:
        ring_buffer(size_t capacity)
            : slots_(capacity)
        {
        }

        size_t size() const
        {
            if (tail_ >= head_)
            {
                return tail_ - head_;
            }
            else
            {
                return ((head_ + capacity() - 1) - tail_);
            }
        }

        auto capacity() const -> size_t { return slots_.size(); }

        auto read() -> std::optional<Value>
        {
            if (head_ == tail_)
            {
                return {};
            }

            auto res = std::make_optional(std::move(slots_[head_].val()));

            ++head_;
            if (head_ == capacity())
            {
                head_ = 0;
            }

            return res;
        }

        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            if (head_ == ((tail_ + 1) % slots_.size()))
            {
                // Destruct old value
                slots_[head_].val().~Value();
            }

            auto & s = slots_[tail_];
            new(s.data_) Value(std::forward<Args>(args)...);
            ++tail_;
            if (tail_ == slots_.size())
            {
                tail_ = 0;
            }

            // Move head forward
            if (head_ == tail_)
            {
                ++head_;
                if (head_ == capacity())
                {
                    head_ = 0;
                }
            }
        }
    };
}
