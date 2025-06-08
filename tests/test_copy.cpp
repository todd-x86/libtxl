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
    auto read_impl(txl::buffer_ref buf) -> txl::result<size_t> override
    {
        auto it = txl::make_circular_iterator(std::begin(NUMBERS), std::end(NUMBERS));
        auto to_copy = std::min(max_ - read_, buf.size());
        txl::iter_copy_n(std::next(it, read_ % NUMBERS.size()), to_copy, reinterpret_cast<char *>(buf.begin()));
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
protected:
    auto write_impl(txl::buffer_ref buf) -> txl::result<size_t> override
    {
        std::copy_n(reinterpret_cast<char const *>(buf.begin()), buf.size(), std::back_inserter(available_));
        return buf.size();
    }
public:
    auto available() const -> txl::byte_vector const & { return available_; }
};

TXL_UNIT_TEST(copy_read)
{
    using namespace std::literals;

    auto rd = dummy_reader{100};
    auto wr = dummy_writer{};

    auto copied = txl::copy(rd, wr, txl::exactly{11}).or_throw();
    assert_equal(copied, 11);
    {
        auto expected = txl::buffer_ref{"01234567890"sv};
        auto actual = txl::buffer_ref{wr.available()};
        for (auto c : wr.available())
        {
            std::cout << "'" << c << "', ";
        }
        std::cout << std::endl;
        assert_equal(expected.size(), actual.size());
        assert_true(expected.equal(actual));
    }
}


TXL_RUN_TESTS()
