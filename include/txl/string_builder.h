#pragma once

namespace txl
{
    class string_builder final
    {
    private:
        void * data_ = nullptr;
        size_t num_used_ = 0;
        size_t size_ = 0;
        size_t original_size_;
    public:
        string_builder(size_t block_size = 256)
            : original_size_{block_size}
        {
        }

        ~string_builder()
        {
            free();
        }

        auto append(char const * s)
        {
            auto size = std::strlen(s);
            if (num_used_ + size > size_)
            {
                size_ = size_ + original_size_;
                data_ = std::realloc(data_, size_);
            }
            std::memcpy(static_cast<char *>(data_) + num_used_, s, size);

            num_used_ += size;
        }

        auto free()
        {
            std::free(data_);
            data_ = nullptr;
        }

        auto clear()
        {
            num_used_ = 0;
        }

        auto to_string() const
        {
            return std::string{static_cast<char const *>(data_), num_used_};
        }
    };
}
