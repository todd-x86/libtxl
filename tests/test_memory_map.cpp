#include <txl/unit_test.h>
#include <txl/memory_map.h>

#include <string_view>

using namespace std::literals;

TXL_UNIT_TEST(mmap_anonymous)
{
    auto map = txl::memory_map{};
    {
        auto m = map.memory();
        assert_true(m.empty());
    }
    assert_false(map.is_open());
    map.open(4096 * 8, txl::memory_map::read | txl::memory_map::write).or_throw();
    assert_true(map.is_open());

    {
        auto m = map.memory();
        assert_equal(map.size(), m.size());

        m = "Hello World"sv;
    }
    {
        auto m = map.memory();
        assert_equal(m.slice(0, "Hello World"sv.size()).to_string_view(), "Hello World"sv);
        assert_equal(std::byte{'H'}, m[0]);
        assert_equal(std::byte{'e'}, m[1]);
        assert_equal(std::byte{'l'}, m[2]);
        assert_equal(std::byte{'l'}, m[3]);
        assert_equal(std::byte{'o'}, m[4]);

        // "Hello" -> "Hallo"
        m[1] = std::byte{'a'};
        assert_equal(m.slice(0, 5).to_string_view(), "Hallo"sv);

        assert_false(map.sync(m).is_error());
    }
}

TXL_RUN_TESTS()
