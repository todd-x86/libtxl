#include <txl/unit_test.h>
#include <txl/atomic.h>
#include <txl/threading.h>

#include <thread>
#include <string>
#include <atomic>

TXL_UNIT_TEST(atomic_swap_int)
{
    std::atomic<int> value{10};
    int old_value = txl::atomic_swap(value, 20);
    
    assert_equal(old_value, 10);
    assert_equal(value.load(), 20);
}

TXL_UNIT_TEST(atomic_swap_with_factory)
{
    std::atomic<int> value{5};
    auto old_value = txl::atomic_swap(value, [](int old) {
        return old * 2;
    });
    
    assert_equal(old_value, 5);
    assert_equal(value.load(), 10);
}

TXL_UNIT_TEST(atomic_swap_if_true)
{
    std::atomic<int> value{100};
    auto old_value = txl::atomic_swap_if(value, 200, [](int v) {
        return v > 50;
    });
    
    assert_equal(old_value, 100);
    assert_equal(value.load(), 200);
}

TXL_UNIT_TEST(atomic_swap_if_false)
{
    std::atomic<int> value{30};
    auto old_value = txl::atomic_swap_if(value, 200, [](int v) {
        return v > 50;
    });
    
    assert_equal(old_value, 30);
    assert_equal(value.load(), 30);
}

TXL_UNIT_TEST(atomic_swap_if_with_factory)
{
    std::atomic<int> value{7};
    auto old_value = txl::atomic_swap_if(value, [](int old) {
        return old * 3;
    }, [](int v) {
        return v < 10;
    });
    
    assert_equal(old_value, 7);
    assert_equal(value.load(), 21);
}

TXL_UNIT_TEST(atomic_swap_if_factory_false)
{
    std::atomic<int> value{15};
    auto old_value = txl::atomic_swap_if(value, [](int old) {
        return old * 3;
    }, [](int v) {
        return v < 10;
    });
    
    assert_equal(old_value, 15);
    assert_equal(value.load(), 15);
}

TXL_UNIT_TEST_N(atomic_swap_concurrency, 50)
{
    std::atomic<int> value{0};
    auto barrier = txl::awaiter{};
    std::atomic<int> swap_count{0};

    auto swapper = [&]() {
        barrier.wait();
        txl::atomic_swap(value, [](int v) {
            return v + 1;
        });
        swap_count.fetch_add(1, std::memory_order_acq_rel);
    };

    constexpr int num_threads = 8;
    std::thread threads[num_threads];

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread{swapper};
    }

    barrier.set();

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i].join();
    }

    // All threads should have successfully swapped
    assert_equal(swap_count.load(), num_threads);
    // Value should have incremented (at least once, likely more due to contention)
    assert_true(value.load() >= 1);
}

// ============================================================================
// acquire_lock Tests
// ============================================================================

TXL_UNIT_TEST(acquire_lock_basic)
{
    std::atomic<int> value{10};
    {
        txl::acquire_lock<int> lock{value};
        assert_equal(lock.value(), 10);
        lock.value() = 20;
    }
    
    // After scope, value should be updated
    assert_equal(value.load(), 20);
}

TXL_UNIT_TEST(acquire_lock_no_change)
{
    std::atomic<int> value{10};
    {
        txl::acquire_lock<int> lock{value};
        assert_equal(lock.value(), 10);
        // Don't modify, so CAS should not occur
    }
    
    // Value should remain unchanged
    assert_equal(value.load(), 10);
}

TXL_UNIT_TEST(acquire_lock_ptr_access)
{
    std::atomic<int> value{42};
    {
        txl::acquire_lock<int> lock{value};
        *lock = 84;
        assert_equal(*lock, 84);
    }
    
    assert_equal(value.load(), 84);
}

TXL_UNIT_TEST(acquire_lock_arrow_operator)
{
    struct Point final
    {
        int x, y;
        
        Point() = default;
        Point(int x, int y) : x{x}, y{y} {}
        
        auto operator==(Point const & other) const -> bool
        {
            return x == other.x and y == other.y;
        }
        
        auto operator!=(Point const & other) const -> bool
        {
            return not (*this == other);
        }
    };

    std::atomic<Point> point;
    point.store(Point{1, 2});

    {
        txl::acquire_lock<Point> lock{point};
        lock->x = 10;
        lock->y = 20;
        assert_equal(lock->x, 10);
        assert_equal(lock->y, 20);
    }

    auto final_point = point.load();
    assert_equal(final_point.x, 10);
    assert_equal(final_point.y, 20);
}

TXL_UNIT_TEST_N(acquire_lock_concurrency, 50)
{
    std::atomic<int> counter{0};
    auto barrier = txl::awaiter{};
    std::atomic<int> success_count{0};

    auto incrementer = [&]() {
        barrier.wait();
        for (auto i = 0; i < 10; ++i)
        {
            txl::acquire_lock<int> lock{counter};
            auto old_val = lock.value();
            lock.value() = old_val + 1;
        }
        success_count.fetch_add(1, std::memory_order_acq_rel);
    };

    constexpr int num_threads = 4;
    std::thread threads[num_threads];

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread{incrementer};
    }

    barrier.set();

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i].join();
    }

    // All threads should have completed
    assert_equal(success_count.load(), num_threads);
    // Counter should have been incremented (at least once)
    assert_true(counter.load() > 0);
}

TXL_UNIT_TEST(lazy_atomic_simple_int_ptr)
{
    auto init_count = 0;
    auto lazy_int = txl::lazy_atomic<int>{[&]() {
        ++init_count;
        return new int{42};
    }};

    // Before accessing, factory should not have been called
    assert_equal(init_count, 0);

    // First access triggers initialization
    auto * ptr = lazy_int.ptr();
    assert_true(ptr != nullptr);
    assert_equal(*ptr, 42);
    assert_equal(init_count, 1);
}

TXL_UNIT_TEST(lazy_atomic_simple_string_ptr)
{
    auto init_count = 0;
    auto lazy_str = txl::lazy_atomic<std::string>{[&]() {
        ++init_count;
        return new std::string{"Test String"};
    }};

    // Before accessing, factory should not have been called
    assert_equal(init_count, 0);

    // First access triggers initialization
    auto * ptr = lazy_str.ptr();
    assert_true(ptr != nullptr);
    assert_equal(*ptr, "Test String");
    assert_equal(init_count, 1);
}

TXL_UNIT_TEST_N(lazy_atomic_concurrency_read_int, 50)
{
    auto lazy_int = txl::lazy_atomic<int>{[]() {
        return new int(42);
    }};

    auto barrier = txl::awaiter{};
    std::atomic<int> successful_reads{0};

    auto reader = [&]() {
        barrier.wait();
        auto * ptr = lazy_int.ptr();
        if (ptr and *ptr == 42)
        {
            successful_reads.fetch_add(1, std::memory_order_acq_rel);
        }
    };

    constexpr int num_threads = 8;
    std::thread threads[num_threads];

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread{reader};
    }

    barrier.set();

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i].join();
    }

    assert_equal(successful_reads.load(), num_threads);
}

TXL_UNIT_TEST_N(lazy_atomic_concurrency_read_string, 50)
{
    auto lazy_str = txl::lazy_atomic<std::string>{[]() {
        return new std::string{"Concurrent String Value"};
    }};

    auto barrier = txl::awaiter{};
    std::atomic<int> successful_reads{0};

    auto reader = [&]() {
        barrier.wait();
        auto * str_ptr = lazy_str.ptr();
        if (str_ptr and *str_ptr == "Concurrent String Value")
        {
            successful_reads.fetch_add(1, std::memory_order_acq_rel);
        }
    };

    constexpr int num_threads = 10;
    std::thread threads[num_threads];

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread{reader};
    }

    barrier.set();

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i].join();
    }

    // All threads should have successfully read the string
    assert_equal(successful_reads.load(), num_threads);
}

TXL_UNIT_TEST_N(lazy_atomic_concurrency_mixed_access, 100)
{
    auto init_count = std::make_shared<std::atomic<int>>(0);
    auto lazy_val = txl::lazy_atomic<int>{[init_count]() {
        init_count->fetch_add(1, std::memory_order_acq_rel);
        return new int{999};
    }};

    auto barrier = txl::awaiter{};
    std::atomic<int> successful_accesses{0};

    auto accessor = [&]() {
        barrier.wait();
        // Use ptr() method
        auto * ptr = lazy_val.ptr();
        if (ptr and *ptr == 999)
        {
            successful_accesses.fetch_add(1, std::memory_order_acq_rel);
        }
    };

    constexpr int num_threads = 16;
    std::thread threads[num_threads];

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread{accessor};
    }

    barrier.set();

    for (auto i = 0; i < num_threads; ++i)
    {
        threads[i].join();
    }

    // All threads should have succeeded
    assert_equal(successful_accesses.load(), num_threads);
    // Factory should have been called exactly once
    assert_equal(init_count->load(), 1);
}

TXL_UNIT_TEST(lazy_atomic_int_get_method)
{
    auto init_count = 0;
    auto lazy_int = txl::lazy_atomic<int>{[&]() {
        ++init_count;
        return new int{100};
    }};

    // Before accessing, factory should not have been called
    assert_equal(init_count, 0);

    // First access via get() triggers initialization
    auto & ref1 = lazy_int.get();
    assert_equal(ref1, 100);
    assert_equal(init_count, 1);

    // Subsequent access returns same reference
    auto & ref2 = lazy_int.get();
    assert_equal(ref2, 100);
    // Factory should still have been called only once
    assert_equal(init_count, 1);

    // Modify the value and verify persistence
    ref1 = 200;
    assert_equal(lazy_int.get(), 200);
}

TXL_UNIT_TEST(lazy_atomic_int_modification)
{
    auto lazy_int = txl::lazy_atomic<int>{[]() {
        return new int{50};
    }};

    auto * ptr = lazy_int.ptr();
    assert_equal(*ptr, 50);

    // Modify the value
    *ptr = 75;

    // Verify the modification persists
    assert_equal(*lazy_int.ptr(), 75);
}

TXL_UNIT_TEST(lazy_atomic_string_basic)
{
    auto init_count = 0;
    auto lazy_str = txl::lazy_atomic<std::string>{[&]() {
        ++init_count;
        return new std::string{"Hello, World!"};
    }};

    // Before accessing, factory should not have been called
    assert_equal(init_count, 0);

    // First access triggers initialization
    auto * ptr1 = lazy_str.ptr();
    assert_equal(*ptr1, "Hello, World!");
    assert_equal(init_count, 1);

    // Subsequent accesses return the same pointer
    auto * ptr2 = lazy_str.ptr();
    assert_equal(*ptr2, "Hello, World!");
    // Factory should still have been called only once
    assert_equal(init_count, 1);

    // Both pointers should be identical
    assert_true(ptr1 == ptr2);
}

TXL_UNIT_TEST(lazy_atomic_string_get)
{
    auto lazy_str = txl::lazy_atomic<std::string>{[]() {
        return new std::string{"Initial"};
    }};

    // First access should give us the initial value
    auto * ptr = lazy_str.ptr();
    assert_equal(*ptr, "Initial");

    // Modify the string through the pointer
    ptr->append(" Value");
    
    // Verify the modification persists through get()
    auto & ref = lazy_str.get();
    assert_equal(ref, "Initial Value");
}

TXL_UNIT_TEST(lazy_atomic_string_mutation)
{
    auto lazy_str = txl::lazy_atomic<std::string>{[]() {
        return new std::string{"Test"};
    }};

    // Get pointer and modify
    auto * str_ptr = lazy_str.ptr();
    assert_equal(*str_ptr, "Test");
    
    str_ptr->append(" String");
    assert_equal(*str_ptr, "Test String");

    // Verify through get() method
    assert_equal(lazy_str.get(), "Test String");
}

TXL_UNIT_TEST(lazy_atomic_custom_type)
{
    struct ComplexType final
    {
        int id;
        std::string name;
        double value;

        ComplexType(int id, std::string const & name, double value)
            : id{id}, name{name}, value{value} {}
    };

    auto init_count = 0;
    auto lazy_obj = txl::lazy_atomic<ComplexType>{[&]() {
        ++init_count;
        return new ComplexType{123, "Test Object", 3.14159};
    }};

    assert_equal(init_count, 0);

    auto * obj_ptr = lazy_obj.ptr();
    assert_equal(obj_ptr->id, 123);
    assert_equal(obj_ptr->name, "Test Object");
    assert_true(obj_ptr->value > 3.14 && obj_ptr->value < 3.15);
    assert_equal(init_count, 1);

    // Verify get() returns same object
    auto & obj_ref = lazy_obj.get();
    assert_equal(obj_ref.id, 123);
    // Factory should still have been called only once
    assert_equal(init_count, 1);
}

TXL_UNIT_TEST(lazy_atomic_custom_type_modification)
{
    struct Data final
    {
        int count = 0;
        std::string text = "";
    };

    auto lazy_data = txl::lazy_atomic<Data>{[]() {
        return new Data{};
    }};

    auto * data_ptr = lazy_data.ptr();
    assert_equal(data_ptr->count, 0);
    
    data_ptr->count = 42;
    data_ptr->text = "Modified";

    // Verify modifications persist
    assert_equal(lazy_data.get().count, 42);
    assert_equal(lazy_data.get().text, "Modified");
}

TXL_UNIT_TEST(lazy_atomic_assignment_int)
{
    auto lazy_int = txl::lazy_atomic<int>{[]() {
        return new int{10};
    }};

    // Get initial value
    assert_equal(*lazy_int.ptr(), 10);

    // Assign new value via move
    lazy_int = 42;

    // Verify new value is stored
    assert_equal(*lazy_int.ptr(), 42);
}

TXL_UNIT_TEST(lazy_atomic_assignment_string)
{
    auto lazy_str = txl::lazy_atomic<std::string>{[]() {
        return new std::string{"initial"};
    }};

    // Get initial value
    assert_equal(*lazy_str.ptr(), "initial");

    // Assign new value via move
    lazy_str = "replaced";

    // Verify new value is stored
    assert_equal(*lazy_str.ptr(), "replaced");
}

TXL_UNIT_TEST(lazy_atomic_assignment_replaces_old)
{
    auto init_count = 0;
    auto lazy_int = txl::lazy_atomic<int>{[&]() {
        ++init_count;
        return new int{100};
    }};

    // Initialize with factory
    auto * ptr1 = lazy_int.ptr();
    assert_equal(*ptr1, 100);
    assert_equal(init_count, 1);

    // Assign new value, which should clear the old one
    lazy_int = 200;

    // Verify new value
    auto * ptr2 = lazy_int.ptr();
    assert_equal(*ptr2, 200);
}

TXL_UNIT_TEST(lazy_atomic_assignment_custom_type)
{
    struct Point final
    {
        int x, y;
        Point(int x = 0, int y = 0) : x{x}, y{y} {}
    };

    auto lazy_point = txl::lazy_atomic<Point>{[]() {
        return new Point{1, 2};
    }};

    // Initial value
    assert_equal(lazy_point.ptr()->x, 1);
    assert_equal(lazy_point.ptr()->y, 2);

    // Assign new Point
    lazy_point = Point{10, 20};

    // Verify new values
    assert_equal(lazy_point.ptr()->x, 10);
    assert_equal(lazy_point.ptr()->y, 20);
}

TXL_UNIT_TEST(lazy_atomic_assignment_multiple_times)
{
    auto lazy_str = txl::lazy_atomic<std::string>{[]() {
        return new std::string{"first"};
    }};

    assert_equal(*lazy_str.ptr(), "first");

    lazy_str = "second";
    assert_equal(*lazy_str.ptr(), "second");

    lazy_str = "third";
    assert_equal(*lazy_str.ptr(), "third");

    lazy_str = "fourth";
    assert_equal(*lazy_str.ptr(), "fourth");
}

TXL_UNIT_TEST(lazy_atomic_assignment_clears_previous)
{
    auto destroy_count = 0;
    
    struct TrackedValue final
    {
        int id;
        int * destroy_counter;
        
        TrackedValue(int id, int * counter) : id{id}, destroy_counter{counter} {}

        ~TrackedValue()
        { 
            if (destroy_counter)
            {
                ++(*destroy_counter);
            }
        }
    };

    auto lazy_val = txl::lazy_atomic<TrackedValue>{[&]() {
        return new TrackedValue{1, &destroy_count};
    }};

    // Initialize
    assert_equal(lazy_val.ptr()->id, 1);
    assert_equal(destroy_count, 0);

    // Assign new value (old should be destroyed)
    lazy_val = TrackedValue{2, &destroy_count};
    assert_equal(lazy_val.ptr()->id, 2);
    assert_equal(destroy_count, 2);  // Old value was destroyed

    // Assign again
    lazy_val = TrackedValue{3, &destroy_count};
    assert_equal(lazy_val.ptr()->id, 3);
    assert_equal(destroy_count, 4);  // Another old value was destroyed
}

TXL_UNIT_TEST(lazy_atomic_assignment_with_get)
{
    auto lazy_int = txl::lazy_atomic<int>{[]() {
        return new int{5};
    }};

    // Access via get()
    auto & ref1 = lazy_int.get();
    assert_equal(ref1, 5);

    // Assign new value
    lazy_int = 15;

    // get() should return new value
    auto & ref2 = lazy_int.get();
    assert_equal(ref2, 15);
}

TXL_UNIT_TEST_N(lazy_atomic_assignment_concurrency, 50)
{
    auto lazy_int = txl::lazy_atomic<int>{[]() {
        return new int{0};
    }};

    auto barrier = txl::awaiter{};
    std::atomic<int> assignment_count{0};
    std::atomic<int> successful_reads{0};

    auto assigner = [&]() {
        barrier.wait();
        // Each thread assigns a new value
        lazy_int = 42 + assignment_count.fetch_add(1, std::memory_order_acq_rel);
    };

    auto reader = [&]() {
        barrier.wait();
        // Small delay to ensure assignments happen
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto * ptr = lazy_int.ptr();
        if (ptr and *ptr >= 42)
        {
            successful_reads.fetch_add(1, std::memory_order_acq_rel);
        }
    };

    constexpr int num_assigners = 4;
    constexpr int num_readers = 4;
    std::thread assigners[num_assigners];
    std::thread readers[num_readers];

    for (auto i = 0; i < num_assigners; ++i)
    {
        assigners[i] = std::thread{assigner};
    }

    for (auto i = 0; i < num_readers; ++i)
    {
        readers[i] = std::thread{reader};
    }

    barrier.set();

    for (auto i = 0; i < num_assigners; ++i)
    {
        assigners[i].join();
    }

    for (auto i = 0; i < num_readers; ++i)
    {
        readers[i].join();
    }

    // At least some reads should have succeeded
    assert_true(successful_reads.load() >= 1);
    // Some assignments should have occurred
    assert_true(assignment_count.load() >= 1);
}

TXL_UNIT_TEST(lazy_atomic_assignment_from_temporary)
{
    auto lazy_str = txl::lazy_atomic<std::string>{[]() {
        return new std::string{"original"};
    }};

    assert_equal(*lazy_str.ptr(), "original");

    // Assign from temporary
    lazy_str = "temporary";
    assert_equal(*lazy_str.ptr(), "temporary");

    // Assign from expression
    lazy_str = std::string("hello") + std::string(" world");
    assert_equal(*lazy_str.ptr(), "hello world");
}

TXL_RUN_TESTS()
