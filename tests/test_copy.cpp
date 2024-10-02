#include <txl/unit_test.h>
#include <txl/io.h>
#include <txl/iterators.h>
#include <txl/copy.h>
#include <txl/read_string.h>
#include <txl/types.h>

#include <string_view>
#include <algorithm>

static constexpr const std::string_view NUMBERS = "0123456789";

struct dummy_reader final : txl::reader
{
private:
    size_t read_ = 0;
    size_t max_;
protected:
    auto read_impl(txl::buffer_ref buf, txl::on_error::callback<txl::system_error> on_err) -> size_t override
    {
        auto it = txl::make_circular_iterator(std::begin(NUMBERS), std::end(NUMBERS));
        auto to_copy = std::min(max_ - read_, buf.size());
        std::copy_n(std::next(it, read_ % NUMBERS.size()), to_copy, reinterpret_cast<char *>(buf.begin()));
        return to_copy;
    }
public:
    dummy_reader(size_t max)
        : max_(max)
    {
    }
};

struct dummy_writer final : txl::writer
{
private:
    txl::byte_vector available_;
    size_t written_ = 0;
    size_t max_;
protected:
    auto write_impl(txl::buffer_ref buf, txl::on_error::callback<txl::system_error> on_err) -> size_t override
    {
        auto to_copy = std::min(max_ - written_, buf.size());
        std::copy_n(std::next(std::begin(NUMBERS), written_ % NUMBERS.size()), to_copy, std::back_inserter(available_));
        return to_copy;
    }
public:
    dummy_writer(size_t max)
        : max_(max)
    {
    }

    auto available() const -> txl::byte_vector const & { return available_; }
};

TXL_UNIT_TEST(copy_read)
{
    auto rd = dummy_reader{100};

    auto wr = dummy_writer{100};
    txl::copy(rd, wr, txl::exactly{11});
}


TXL_RUN_TESTS()
