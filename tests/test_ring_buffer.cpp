#include <txl/unit_test.h>
#include <txl/ring_buffer.h>

struct test
{
    static int ctor;
    static int dtor;

    test(int arg)
    {
        ++ctor;
    }

    ~test()
    {
        ++dtor;
    }
};

int test::ctor = 0;
int test::dtor = 0;

TXL_UNIT_TEST(ring_buffer)
{
    txl::ring_buffer<test> rb(3);
    assert(test::ctor == 0);
    assert(test::dtor == 0);

    rb.emplace(999);
    assert(test::ctor == 1);
    assert(test::dtor == 0);

    rb.emplace(998);
    assert(test::ctor == 2);
    assert(test::dtor == 0);
    
    rb.emplace(997);
    assert(test::ctor == 3);
    assert(test::dtor == 1);
    
    rb.emplace(996);
    assert(test::ctor == 4);
    assert(test::dtor == 2);
}

TXL_UNIT_TEST(ring_buffer_pod)
{
    txl::ring_buffer<bool> rb(3);
    rb.emplace(true);
    rb.emplace(true);
    rb.emplace(true);
    rb.emplace(false);
}

TXL_UNIT_TEST(ring_buffer_size)
{
    txl::ring_buffer<int> rb(3);
    assert(rb.size() == 0);

    rb.emplace(1);
    assert(rb.size() == 1);
    
    rb.emplace(1);
    rb.emplace(1);
    assert(rb.size() == 3);
    
    rb.emplace(1);
    assert(rb.size() == 3);
    
    rb.emplace(1);
    rb.emplace(1);
    assert(rb.size() == 3);
}

TXL_UNIT_TEST(ring_buffer_read)
{
    txl::ring_buffer<int> rb(4);

    rb.emplace(1);
    rb.emplace(2);
    rb.emplace(3);

    assert_equal(1, *rb.read());
    assert_equal(2, *rb.read());
    assert_equal(3, *rb.read());
    assert_equal(std::optional<int>{}, rb.read());
    assert_equal(std::optional<int>{}, rb.read());
    assert_equal(std::optional<int>{}, rb.read());
    
    rb.emplace(4);
    rb.emplace(5);
    assert_equal(4, *rb.read());

    // 5, 6, 7 in buffer
    rb.emplace(6);
    rb.emplace(7);
    assert_equal(5, *rb.read());

    // 7, 8, 9 in buffer
    rb.emplace(8);
    rb.emplace(9);
    assert_equal(7, *rb.read());
}

TXL_RUN_TESTS()
