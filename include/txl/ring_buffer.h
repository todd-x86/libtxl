#pragma once

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
            bool emplaced_ = false;

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
        size_t capacity() const { return slots_.size(); }

        template<class... Args>
        void emplace(Args && ... args)
        {
            auto & s = slots_[tail_];
            if (s.emplaced_)
            {
                // Destruct old one
                s.val().~Value();
            }
            new(s.data_) Value(std::forward<Args>(args)...);
            s.emplaced_ = true;
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
