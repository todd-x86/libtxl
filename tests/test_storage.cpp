#include <txl/unit_test.h>
#include <txl/storage.h>

struct sentinel final
{
    static size_t num_creates_;
    static size_t num_deletes_;

    static auto reset() -> void
    {
        num_creates_ = 0;
        num_deletes_ = 0;
    }

    sentinel()
    {
        ++num_creates_;
    }

    ~sentinel()
    {
        ++num_deletes_;
    }
};

size_t sentinel::num_creates_ = 0;
size_t sentinel::num_deletes_ = 0;

TXL_UNIT_TEST(safe_storage)
{
    sentinel::reset();
    assert_equal(sentinel::num_creates_, 0);
    assert_equal(sentinel::num_deletes_, 0);

    txl::safe_storage<sentinel> empty{};
    assert_true(empty.empty());
    assert_equal(sentinel::num_creates_, 0);
    assert_equal(sentinel::num_deletes_, 0);

    txl::safe_storage<sentinel> once{};
    assert_true(once.empty());
    once.emplace();
    assert_false(once.empty());
    assert_equal(sentinel::num_creates_, 1);
    assert_equal(sentinel::num_deletes_, 0);
    
    once.erase();
    assert_equal(sentinel::num_creates_, 1);
    assert_equal(sentinel::num_deletes_, 1);
    
    once.emplace();
    assert_equal(sentinel::num_creates_, 2);
    assert_equal(sentinel::num_deletes_, 1);
    
    once = txl::safe_storage<sentinel>{};
    assert_equal(sentinel::num_creates_, 2);
    assert_equal(sentinel::num_deletes_, 2);
}

TXL_UNIT_TEST(storage)
{
    sentinel::reset();
    assert_equal(sentinel::num_creates_, 0);
    assert_equal(sentinel::num_deletes_, 0);

    txl::storage<sentinel> empty{};
    assert_equal(sentinel::num_creates_, 0);
    assert_equal(sentinel::num_deletes_, 0);

    empty.emplace();
    assert_equal(sentinel::num_creates_, 1);
    assert_equal(sentinel::num_deletes_, 0);
    
    empty.emplace();
    assert_equal(sentinel::num_creates_, 2);
    assert_equal(sentinel::num_deletes_, 0);
    
    empty.erase();
    assert_equal(sentinel::num_creates_, 2);
    assert_equal(sentinel::num_deletes_, 1);
    
    empty.erase();
    assert_equal(sentinel::num_creates_, 2);
    assert_equal(sentinel::num_deletes_, 2);
}

struct thingy
{
    std::string name;
    double value;
    int x;
};

TXL_UNIT_TEST(size)
{
    assert_equal(sizeof(txl::storage<thingy>), sizeof(thingy));
}

TXL_RUN_TESTS()
