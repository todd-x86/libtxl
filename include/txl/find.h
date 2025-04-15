#pragma once

#include <txl/buffer_ref.h>

namespace txl
{
    class stream_find final
    {
    private:
        buffer_ref match_;
        // KMP LPS table
        std::vector<size_t> lps_;
        size_t match_index_ = 0;
        size_t bytes_processed_ = 0;

        auto init_lps_table() -> void
        {
            size_t last = 0;
            size_t next = 1;
            while (next < match_.size())
            {
                if (match_[last] == match_[next])
                {
                    // Advance indices if there's a match
                    lps_[next] = last+1;
                    ++last;
                    ++next;
                }
                else
                {
                    if (last == 0)
                    {
                        // No matches since the beginning
                        lps_[next] = 0;
                        ++next;
                    }
                    else
                    {
                        // Walk back one match index
                        last = lps_[last-1];
                    }
                }
            }
        }
    public:
        stream_find(buffer_ref match)
            : match_{match}
            , lps_{match.size(), 0}
        {
            init_lps_table();
        }

        auto reset() -> void
        {
            match_index_ = 0;
            bytes_processed_ = 0;
        }

        auto process(buffer_ref src)
        {
            size_t s_index = 0;
            while (not is_matched() and s_index < src.size())
            {
                if (src[s_index] == match_[match_index_])
                {
                    ++match_index_;
                    ++bytes_processed_;
                    ++s_index;
                }
                else
                {
                    match_index_ = lps_[match_index_-1];
                    if (match_index_ == 0)
                    {
                        // No substring to skip
                        ++s_index;
                        ++bytes_processed_;
                    }
                }
            }
        }

        auto num_bytes_processed() const -> size_t { return bytes_processed_; }

        auto is_matched() const -> bool
        {
            return match_index_ == match_.size();
        }
    };
}
