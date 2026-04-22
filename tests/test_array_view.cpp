#include <txl/unit_test.h>
#include <txl/array_view.h>

// Default constructor
TXL_UNIT_TEST(array_view_default_constructor)
{
    txl::array_view<int> view;
    assert_true(view.empty());
    assert_equal(view.size(), 0);
}

// Pointer range constructor
TXL_UNIT_TEST(array_view_pointer_range_constructor)
{
    int data[] = {1, 2, 3, 4, 5};
    txl::array_view<int> view(data, data + 5);
    
    assert_false(view.empty());
    assert_equal(view.size(), 5);
    assert_equal(view[0], 1);
    assert_equal(view[4], 5);
}

// Single value constructor
TXL_UNIT_TEST(array_view_single_value_constructor)
{
    int value = 42;
    txl::array_view<int> view(value);
    
    assert_false(view.empty());
    assert_equal(view.size(), 1);
    assert_equal(view[0], 42);
}

// std::array constructor
TXL_UNIT_TEST(array_view_std_array_constructor)
{
    std::array<int, 3> data = {10, 20, 30};
    txl::array_view<int> view(data);
    
    assert_false(view.empty());
    assert_equal(view.size(), 3);
    assert_equal(view[0], 10);
    assert_equal(view[1], 20);
    assert_equal(view[2], 30);
}

// Fixed-size array constructor
TXL_UNIT_TEST(array_view_fixed_array_constructor)
{
    int data[] = {100, 200, 300};
    txl::array_view<int> view(data);
    
    assert_false(view.empty());
    assert_equal(view.size(), 3);
    assert_equal(view[0], 100);
    assert_equal(view[1], 200);
    assert_equal(view[2], 300);
}

// Element access with operator[]
TXL_UNIT_TEST(array_view_operator_bracket)
{
    int data[] = {5, 10, 15, 20, 25};
    txl::array_view<int> view(data);
    
    for (size_t i = 0; i < view.size(); ++i)
    {
        assert_equal(view[i], static_cast<int>(5 + i * 5));
    }
}

// Mutable element access with operator[]
TXL_UNIT_TEST(array_view_operator_bracket_mutable)
{
    int data[] = {1, 2, 3, 4, 5};
    txl::array_view<int> view(data);
    
    view[0] = 100;
    view[2] = 300;
    view[4] = 500;
    
    assert_equal(data[0], 100);
    assert_equal(data[2], 300);
    assert_equal(data[4], 500);
}

// data() method
TXL_UNIT_TEST(array_view_data)
{
    int data[] = {1, 2, 3, 4, 5};
    txl::array_view<int> view(data);
    
    assert_equal(*view.data(), 1);
    assert_equal(view.data()[0], 1);
    assert_equal(view.data()[4], 5);
}

// begin() and end() iterators
TXL_UNIT_TEST(array_view_iterators)
{
    int data[] = {1, 2, 3, 4, 5};
    txl::array_view<int> view{data};
    
    assert_equal(*view.begin(), 1);
    assert_equal(*(view.end() - 1), 5);
    
    int sum = 0;
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        sum += *it;
    }
    assert_equal(sum, 15); // 1+2+3+4+5
}

// Const iterators
TXL_UNIT_TEST(array_view_const_iterators)
{
    int data[] = {1, 2, 3, 4, 5};
    const txl::array_view<int> view(data);
    
    assert_equal(view.begin(), data);
    assert_equal(view.end(), data + 5);
    assert_equal(view.cbegin(), data);
    assert_equal(view.cend(), data + 5);
}

// Range-based for loop
TXL_UNIT_TEST(array_view_range_for)
{
    int data[] = {10, 20, 30, 40, 50};
    txl::array_view<int> view(data);
    
    int index = 0;
    for (int val : view)
    {
        assert_equal(val, data[index]);
        ++index;
    }
    assert_equal(index, 5);
}

// size() method
TXL_UNIT_TEST(array_view_size)
{
    int data1[] = {1};
    txl::array_view<int> view1(data1);
    assert_equal(view1.size(), 1);
    
    int data2[] = {1, 2, 3, 4, 5};
    txl::array_view<int> view2(data2);
    assert_equal(view2.size(), 5);
    
    txl::array_view<int> empty_view;
    assert_equal(empty_view.size(), 0);
}

// empty() method
TXL_UNIT_TEST(array_view_empty)
{
    int data[] = {1, 2, 3};
    txl::array_view<int> non_empty(data);
    assert_false(non_empty.empty());
    
    txl::array_view<int> empty_view;
    assert_true(empty_view.empty());
}

// slice() with position and length
TXL_UNIT_TEST(array_view_slice_pos_len)
{
    int data[] = {10, 20, 30, 40, 50};
    txl::array_view<int> view(data);
    
    // Slice from position 1, length 3: should get [20, 30, 40]
    txl::array_view<int> sliced = view.slice(1, 3);
    assert_equal(sliced.size(), 3);
    assert_equal(sliced[0], 20);
    assert_equal(sliced[1], 30);
    assert_equal(sliced[2], 40);
}

// slice() with only position
TXL_UNIT_TEST(array_view_slice_pos_only)
{
    int data[] = {10, 20, 30, 40, 50};
    txl::array_view<int> view(data);
    
    // Slice from position 2 onwards: should get [30, 40, 50]
    txl::array_view<int> sliced = view.slice(2);
    assert_equal(sliced.size(), 3);
    assert_equal(sliced[0], 30);
    assert_equal(sliced[1], 40);
    assert_equal(sliced[2], 50);
}

// slice() edge case: position at end
TXL_UNIT_TEST(array_view_slice_at_end)
{
    int data[] = {10, 20, 30};
    txl::array_view<int> view(data);
    
    txl::array_view<int> sliced = view.slice(3);
    assert_true(sliced.empty());
    assert_equal(sliced.size(), 0);
}

// slice() edge case: position beyond end
TXL_UNIT_TEST(array_view_slice_beyond_end)
{
    int data[] = {10, 20, 30};
    txl::array_view<int> view(data);
    
    txl::array_view<int> sliced = view.slice(10);
    assert_true(sliced.empty());
}

// slice() edge case: length beyond available
TXL_UNIT_TEST(array_view_slice_length_clamped)
{
    int data[] = {10, 20, 30, 40, 50};
    txl::array_view<int> view(data);
    
    // Request slice from position 2 with length 10, should only get [30, 40, 50]
    txl::array_view<int> sliced = view.slice(2, 10);
    assert_equal(sliced.size(), 3);
    assert_equal(sliced[0], 30);
    assert_equal(sliced[1], 40);
    assert_equal(sliced[2], 50);
}

// slice() from position 0 with full length
TXL_UNIT_TEST(array_view_slice_full)
{
    int data[] = {1, 2, 3, 4, 5};
    txl::array_view<int> view(data);
    
    txl::array_view<int> sliced = view.slice(0, 5);
    assert_equal(sliced.size(), view.size());
    for (size_t i = 0; i < sliced.size(); ++i)
    {
        assert_equal(sliced[i], view[i]);
    }
}

// With strings
TXL_UNIT_TEST(array_view_with_strings)
{
    std::string data[] = {"apple", "banana", "cherry", "date"};
    txl::array_view<std::string> view(data);
    
    assert_equal(view.size(), 4);
    assert_equal(view[0], "apple");
    assert_equal(view[2], "cherry");
    
    txl::array_view<std::string> sliced = view.slice(1, 2);
    assert_equal(sliced.size(), 2);
    assert_equal(sliced[0], "banana");
    assert_equal(sliced[1], "cherry");
}

// Const array_view element access
TXL_UNIT_TEST(array_view_const_element_access)
{
    int data[] = {5, 10, 15};
    const txl::array_view<int> view(data);
    
    // This should use the const operator[]
    assert_equal(view[0], 5);
    assert_equal(view[1], 10);
    assert_equal(view[2], 15);
}

// Pointer range with zero length
TXL_UNIT_TEST(array_view_zero_length_range)
{
    int data[] = {1, 2, 3};
    txl::array_view<int> view(data, data);
    
    assert_true(view.empty());
    assert_equal(view.size(), 0);
}

TXL_RUN_TESTS()