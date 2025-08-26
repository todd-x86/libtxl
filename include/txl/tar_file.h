#pragma once

#include <txl/convert.h>
#include <txl/io.h>
#include <txl/numeric.h>
#include <txl/read_string.h>
#include <txl/result.h>
#include <txl/result_error.h>
#include <txl/time.h>
#include <txl/types.h>

#include <chrono>
#include <string>
#include <string_view>

namespace txl
{
    namespace detail
    {
        struct tar_header final
        {
            char name[100];
            char mode[8];
            char uid[8];
            char gid[8];
            char size[12];
            char mtime[12];
            char chksum[8];
            char typeflag;
            char linkname[100];
            char magic[6];
            char version[2];
            char uname[32];
            char gname[32];
            char devmajor[8];
            char devminor[8];
            char prefix[155];
        };

        static_assert(sizeof(tar_header) == 500, "tar_header does not match Tar file specification size");

        static constexpr const size_t BLOCK_SIZE = 512;
        static constexpr const std::string_view LONG_LINK = "././@LongLink";
    }

    enum class tar_error_code
    {
        none = 0,
        partial_read,
        read_error,
        read_string_error,
        octal_decode_error,
    };

    struct tar_error_context
    {
        using error_type = tar_error_code;
        using exception_type = result_error<tar_error_code>;
    };

    template<class T>
    using tar_result = result<T, tar_error_context>;

    class tar_data_reader final : public reader
    {
    private:
        txl::position_reader & reader_;
        off_t next_;
        off_t end_;
    protected:
        auto read_impl(buffer_ref buf) -> result<size_t> override
        {
            auto res = reader_.read(next_, buf.slice(0, end_-next_));
            if (not res.is_error())
            {
                next_ += res->size();
            }
            return res->size();
        }
    public:
        tar_data_reader(txl::position_reader & reader, off_t next, off_t end)
            : reader_{reader}
            , next_{next}
            , end_{end}
        {
        }

        auto num_remaining() const -> size_t { return end_ - next_; }
    };

    class tar_entry final
    {
        friend class tar_reader;
    public:
        enum entry_type
        {
            unknown,
            file,
            directory,
            link,
        };
        
        tar_entry() = default;
        
        auto filename() const -> std::string_view { return name_; }
        auto size() const -> size_t { return size_; }
        auto type() const -> entry_type { return type_; }
        auto modification_time() const -> std::chrono::system_clock::time_point const & { return mod_time_; } 

        auto offset() const -> off_t { return pos_; }

        auto open_data_reader(txl::position_reader & reader) const -> tar_data_reader
        {
            return tar_data_reader{reader, offset(), offset() + static_cast<off_t>(size())};
        }
    private:
        std::string name_;
        size_t size_ = 0;
        entry_type type_ = unknown;
        std::chrono::system_clock::time_point mod_time_;

        off_t pos_ = 0;

        tar_entry(off_t offset)
            : pos_{offset}
        {
        }
    };

    class tar_reader
    {
    private:
        txl::position_reader * reader_ = nullptr;
        off_t next_pos_ = 0;

        struct tar_header final
        {
            detail::tar_header header_;
            off_t pos_;
            size_t size_;
        };
        
        auto read_header() -> tar_result<tar_header>
        {
            tar_header res;
            auto bytes_read = reader_->read(next_pos_, buffer_ref::cast(res.header_));
            if (bytes_read.is_error())
            {
                return {tar_error_code::read_error}; //, bytes_read.error()};
            }
            if (bytes_read->size() == 0)
            {
                // Empty result
                return {};
            }
            if (bytes_read->size() < sizeof(res.header_))
            {
                return tar_error_code::partial_read;
            }
        
            auto entry_size = txl::convert_to<size_t>(txl::octet_string_view{res.header_.size});
            if (entry_size.is_error())
            {
                return tar_error_code::octal_decode_error;
            }

            res.pos_ = next_pos_ + detail::BLOCK_SIZE; // payload offset
            res.size_ = *entry_size;

            auto bytes_to_skip = (detail::BLOCK_SIZE /* header */ + *entry_size);
            next_pos_ += txl::align(bytes_to_skip, detail::BLOCK_SIZE);
            return res;
        }

        static auto get_type(char type) -> tar_entry::entry_type
        {
            switch (type)
            {
                case '\0':
                case '0':
                    return tar_entry::file;
                case '1':
                    return tar_entry::link;
                case '5':
                    return tar_entry::directory;
                default:
                    return tar_entry::unknown;
            }
        }

        static auto convert_to_entry(tar_header const & hdr, std::string && name) -> tar_result<tar_entry>
        {
            tar_entry e{hdr.pos_};
            e.name_ = std::move(name);
            e.size_ = hdr.size_;
            e.type_ = get_type(hdr.header_.typeflag);
            auto mod_time_secs = txl::convert_to<int64_t>(txl::octet_string_view{hdr.header_.mtime});
            if (mod_time_secs.is_error())
            {
                return tar_error_code::octal_decode_error; //mod_time_secs.error();
            }
            e.mod_time_ = txl::time::to_time_point<std::chrono::system_clock::time_point>(std::chrono::seconds{*mod_time_secs});
            return e;
        }
    public:
        tar_reader() = default;

        tar_reader(txl::position_reader & reader)
            : reader_{&reader}
        {
        }

        auto read_entry() -> tar_result<tar_entry>
        {
            auto h = read_header();
            while (h.is_assigned() and not h.is_error() and buffer_ref::cast(h->header_).is_zero())
            {
                h = read_header();
            }

            if (not h.is_assigned())
            {
                return {};
            }
            if (h.is_error())
            {
                return h.error();
            }

            std::string name{};
            if (h->header_.typeflag == 'K' or h->header_.typeflag == 'L')
            {
                // Block sequence: [*current, long filename, file info block]
                // * = you are here
                
                // Follow link for long filename (stored in sequence)
                tar_data_reader rdr{*reader_, h->pos_, h->pos_ + static_cast<off_t>(h->size_) - 1 /* -1 comes from a null-term at the end */};
                auto long_name = read_string(rdr, h->size_);
                if (long_name.is_error())
                {
                    return tar_error_code::read_string_error;
                }
                
                name = std::move(*long_name);

                // Get correct file info block
                h = read_header();
                if (not h.is_assigned())
                {
                    return {};
                }
                if (h.is_error())
                {
                    return h.error();
                }
            }
            else
            {
                name = h->header_.name;
            }
            return convert_to_entry(*h, std::move(name));
        }
    };
}
