#pragma once

#include <txl/buffer_ref.h>

#include <string>
#include <string_view>
#include <algorithm>
#include <vector>
#include <tuple>

namespace txl
{
    class splitter
    {
    private:
        buffer_ref line_;
        size_t size_;
        off_t offset_;
        char split_char_;
    public:
        class const_iterator
        {
        private:
            buffer_ref line_;
            off_t offset_;
            size_t max_length_;
            size_t length_;
            char split_char_;

            void advance()
            {
                offset_ += length_;
                // Skip implicit comma
                if (offset_ < max_length_)
                {
                    ++offset_;
                }
                length_ = 0;
            }

            size_t pos() const
            {
                return offset_ + length_;
            }

            void seek_next()
            {
                while (pos() < max_length_ && (*line_)[pos()] != split_char_)
                {
                    ++length_;
                }
            }

            void move(const_iterator && it)
            {
                std::swap(line_, it.line_);
                std::swap(offset_, it.offset_);
                std::swap(max_length_, it.max_length_);
                std::swap(length_, it.length_);
                std::swap(split_char_, it.split_char_);
            }
        public:
            const_iterator()
                : line_(nullptr)
                , offset_(0)
                , max_length_(0)
                , length_(0)
                , split_char_(',')
            {
            }

            const_iterator(StringType const * data, off_t offset, size_t max_length)
                : const_iterator(data, offset, max_length, ',')
            {
            }

            const_iterator(StringType const * data, off_t offset, size_t max_length, char split_char)
                : line_(data)
                , offset_(offset)
                , max_length_(max_length)
                , length_(0)
                , split_char_(split_char)
            {
                // Find next comma or EOL
                seek_next();
            }

            const_iterator(const_iterator const & it)
                : line_(it.line_)
                , offset_(it.offset_)
                , max_length_(it.max_length_)
                , length_(it.length_)
                , split_char_(it.split_char_)
            {
            }
            
            const_iterator(const_iterator && it)
                : const_iterator()
            {
                move(std::move(it));
            }

            buffer_ref operator*() const
            {
                return StringType(*line_, offset_, length_);
            }

            const_iterator & operator++()
            {
                advance();
                seek_next();
                return *this;
            }

            const_iterator & operator++(int distance)
            {
                ++(*this);
                return *this;
            }

            const_iterator & operator=(const_iterator const & it)
            {
                if (this != std::addressof(it))
                {
                    line_ = it.line_;
                    offset_ = it.offset_;
                    max_length_ = it.max_length_;
                    length_ = it.length_;
                    split_char_ = it.split_char_;
                }
                return *this;
            }

            const_iterator & operator=(const_iterator && it)
            {
                if (this != std::addressof(it))
                {
                    move(std::move(it));
                }
                return *this;
            }

            bool operator==(const_iterator const & it) const
            {
                // NOTE: Avoid checking string contents to make this faster 
                //    -- comparing iterators from different strings is just not right
                return offset_ == it.offset_ 
                    && max_length_ == it.max_length_ 
                    && length_ == it.length_;
            }

            bool operator!=(const_iterator const & it) const
            {
                return !(*this == it);
            }

            off_t offset() const
            {
                return offset_;
            }

            size_t length() const
            {
                return length_;
            }
        };

        splitter(StringType data, size_t size)
            : splitter(data, 0, size)
        {
        }

        splitter(StringType data, off_t offset, size_t size)
            : splitter(data, offset, size, ',')
        {
        }

        splitter(StringType data, off_t offset, size_t size, char split_char)
            : line_(data)
            , size_(size)
            , offset_(offset)
            , split_char_(split_char)
        {
        }

        splitter(splitter const & splitter)
            : splitter(splitter.line_, splitter.offset_, splitter.size_, splitter.split_char_)
        {
        }

        splitter(splitter && splitter)
            : line_(std::move(splitter.line_))
            , size_(0)
            , offset_(0)
            , split_char_(',')
        {
            std::swap(size_, splitter.size_);
            std::swap(offset_, splitter.offset_);
            std::swap(split_char_, splitter.split_char_);
        }

        void reset(StringType data, size_t size)
        {
            reset(data, 0, size);
        }

        void reset(StringType data, off_t offset, size_t size)
        {
            reset(data, offset, size, ',');
        }

        void reset(StringType data, off_t offset, size_t size, char split_char)
        {
            line_ = data;
            size_ = size;
            offset_ = offset;
            split_char_ = split_char;
        }

        const_iterator begin() const
        {
            return cbegin();
        }

        const_iterator end() const
        {
            return cend();
        }

        const_iterator cbegin() const
        {
            return const_iterator(std::addressof(line_), offset_, size_, split_char_);
        }

        const_iterator cend() const
        {
            return const_iterator(std::addressof(line_), offset_ + size_, size_, split_char_);
        }

        splitter & operator=(splitter const & splitter)
        {
            if (this != std::addressof(splitter))
            {
                reset(splitter.line_, splitter.offset_, splitter.size_, splitter.split_char_);
            }
            return *this;
        }

        splitter & operator=(splitter && splitter)
        {
            if (this != std::addressof(splitter))
            {
                line_ = std::move(splitter.line_);
                std::swap(offset_, splitter.offset_);
                std::swap(size_, splitter.size_);
                std::swap(split_char_, splitter.split_char_);
            }
            return *this;
        }
    };

    class row
    {
    private:
        using segment_list = std::vector<std::pair<OffsetType, LengthType>>;
        using segment_iterator = typename segment_list::const_iterator;
        StringType line_;
        segment_list segments_;

        void split(OffsetType offset, LengthType length)
        {
            auto splitter = splitter<StringType>(line_, offset, length);
            for (auto it = splitter.begin(); it != splitter.end(); ++it)
            {
                segments_.emplace_back(it.offset(), it.length());
            }
        }
    public:
        class const_iterator
        {
        private:
            StringType const * line_;
            segment_iterator curr_;
            segment_iterator end_;

            void move(const_iterator && it)
            {
                std::swap(line_, it.line_);
                std::swap(curr_, it.curr_);
                std::swap(end_, it.end_);
            }
        public:
            const_iterator()
                : line_(nullptr)
                , curr_()
                , end_()
            {
            }

            const_iterator(StringType const * line, segment_iterator offset, segment_iterator end)
                : line_(line)
                , curr_(offset)
                , end_(end)
            {
            }

            const_iterator(const_iterator const & it)
                : line_(it.line_)
                , curr_(it.curr_)
                , end_(it.end_)
            {
            }

            const_iterator(const_iterator && it)
                : const_iterator()
            {
                move(std::move(it));
            }

            StringType operator*() const
            {
                OffsetType offset;
                LengthType length;
                std::tie(offset, length) = *curr_;
                return StringType(*line_, offset, length);
            }

            const_iterator & operator++()
            {
                ++curr_;
                return *this;
            }
            
            const_iterator & operator++(int distance)
            {
                ++(*this);
                return *this;
            }

            const_iterator & operator=(const_iterator const & it)
            {
                if (this != std::addressof(it))
                {
                    line_ = it.line_;
                    curr_ = it.curr_;
                    end_ = it.end_;
                }
                return *this;
            }

            const_iterator & operator=(const_iterator && it)
            {
                if (this != std::addressof(it))
                {
                    move(std::move(it));
                }
                return *this;
            }

            bool operator==(const_iterator const & it) const
            {
                // NOTE: Rely on vector iterator comparison (memory address)
                return curr_ == it.curr_;
            }

            bool operator!=(const_iterator const & it) const
            {
                return !(*this == it);
            }
        };

        row(StringType line, LengthType length)
            : row(line, 0, length)
        {
        }

        row(StringType line, OffsetType offset, LengthType length)
            : line_(line)
        {
            split(offset, length);
        }

        row(row const & row)
            : line_(row.line_)
            , segments_(row.segments_)
        {
        }

        row(row && row)
            : line_(std::move(row.line_))
            , segments_(std::move(row.segments_))
        {
        }

        void reset(StringType line, LengthType length)
        {
            reset(line, 0, length);
        }

        void reset(StringType line, OffsetType offset, LengthType length)
        {
            segments_.clear();
            line_ = line;

            split(offset, length);
        }

        StringType operator[](int index) const
        {
            OffsetType offset;
            LengthType length;
            std::tie(offset, length) = segments_[index];
            return StringType(line_, offset, length);
        }

        size_t length() const
        {
            return segments_.size();
        }

        size_t size() const
        {
            return length();
        }

        const_iterator cbegin() const
        {
            return const_iterator(std::addressof(line_), segments_.cbegin(), segments_.cend());
        }

        const_iterator cend() const
        {
            return const_iterator(std::addressof(line_), segments_.cend(), segments_.cend());
        }

        const_iterator begin() const
        {
            return cbegin();
        }

        const_iterator end() const
        {
            return cend();
        }

        row & operator=(row const & row)
        {
            if (this != std::addressof(row))
            {
                line_ = row.line_;
                segments_ = row.segments_;
            }
            return *this;
        }

        row & operator=(row && row)
        {
            if (this != std::addressof(row))
            {
                line_ = std::move(row.line_);
                segments_ = std::move(segments_);
            }
            return *this;
        }
    };
}
