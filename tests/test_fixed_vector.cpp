#include <txl/unit_test.h>
#include <txl/fixed_vector.h>

#include <string>
#include <vector>

TXL_UNIT_TEST(fixed_vector)
{
    txl::fixed_vector<std::string, 3> names{};
    std::vector<std::string> out{};

    assert_equal(names.max_size(), 3);
    assert_equal(names.capacity(), 3);
    assert_equal(out.size(), 0);
    assert_equal(names.size(), out.size());
    for (auto const & name : names)
    {
        out.emplace_back(name);
    }
}

TXL_UNIT_TEST(repeat_construct)
{
    txl::fixed_vector<int, 5> v(3);
    assert_equal(v.size(), 3);
    for (size_t i = 0; i < 3; ++i)
    {
        assert_equal(v[i], 0);
    }
}

TXL_UNIT_TEST(repeat_value_construct)
{
    txl::fixed_vector<int, 5> v(3, 27);
    assert_equal(v.size(), 3);
    for (size_t i = 0; i < 3; ++i)
    {
        assert_equal(v[i], 27);
    }
}

TXL_UNIT_TEST(iter_construct)
{
    std::vector<int> src = {1,2,3};
    txl::fixed_vector<int, 5> v{src.begin(), src.end()};
    assert_equal(v.size(), 3);
    assert_equal(v[0], 1);
    assert_equal(v[1], 2);
    assert_equal(v[2], 3);
}

TXL_UNIT_TEST(copy_construct)
{
    txl::fixed_vector<int, 5> v1(2, 7);
    txl::fixed_vector<int, 5> v2{v1};
    assert_equal(v1.size(), 2);
    assert_equal(v2.size(), 2);
    assert_equal(v1[0], v2[0]);
    assert_equal(v1[1], v2[1]);
}

TXL_UNIT_TEST(move_construct)
{
    txl::fixed_vector<int, 5> v1(2, 8);
    txl::fixed_vector<int, 5> v2(std::move(v1));
    assert_equal(v1.size(), 0);
    assert_equal(v2.size(), 2);
    assert_equal(v2[0], 8);
    assert_equal(v2[1], v2[0]);
}

TXL_UNIT_TEST(initializer_list)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    assert_equal(v.size(), 3);
    assert_equal(v[0], 1);
    assert_equal(v[1], 2);
    assert_equal(v[2], 3);
}

TXL_UNIT_TEST(push_back)
{
    txl::fixed_vector<std::string, 3> names{};
    assert_equal(names.size(), 0);
    names.push_back("Rocko");
    assert_equal(names.size(), 1);
    assert_equal(names[0], "Rocko");

    auto x = std::move(names[0]);
    names.pop_back();
    assert_equal(names.size(), 0);
    assert_equal(x, "Rocko");
}

struct grocery final
{
    std::string name;
    int quantity;
    double price;

    grocery(std::string && name, int quantity, double price)
        : name{std::move(name)}
        , quantity{quantity}
        , price{price}
    {
    }
};

TXL_UNIT_TEST(emplace_back)
{
    txl::fixed_vector<grocery, 8> groceries{};
    groceries.emplace_back("Bread", 1, 2.99);
    groceries.emplace_back("Milk", 2, 1.99);
    groceries.emplace_back("Tomatoes", 8, 1.99);

    assert_equal(groceries.size(), 3);
}

TXL_UNIT_TEST(insert_middle)
{
    txl::fixed_vector<grocery, 8> groceries{};
    groceries.emplace_back("Bread", 1, 2.99);
    groceries.emplace_back("Tomatoes", 8, 1.99);
    groceries.insert(groceries.begin() + 1, grocery{"Milk", 2, 1.99});

    assert_equal(groceries.size(), 3);
    assert_equal(groceries[0].name, "Bread");
    assert_equal(groceries[1].name, "Milk");
    assert_equal(groceries[2].name, "Tomatoes");
}

TXL_UNIT_TEST(copy_assign)
{
    txl::fixed_vector<int, 5> v1(2, 5);
    txl::fixed_vector<int, 5> v2;
    v2 = v1;
    assert_equal(v2.size(), 2);
    assert_equal(v2[1], 5);
    v1[0] = 6;
    assert_equal(v1[0], 6);
    assert_equal(v2[0], 5);
}

TXL_UNIT_TEST(move_assign)
{
    txl::fixed_vector<int, 5> v1(2, 9);
    txl::fixed_vector<int, 5> v2;
    v2 = std::move(v1);
    assert_equal(v2.size(), 2);
    assert_equal(v2[1], 9);
}

TXL_UNIT_TEST(assign_repeat)
{
    txl::fixed_vector<int, 5> v;
    v.assign(3, 11);
    assert_equal(v.size(), 3);
    assert_equal(v[2], 11);
}

TXL_UNIT_TEST(assign_iter)
{
    std::vector<int> src = {4,5};
    txl::fixed_vector<int, 5> v;
    v.assign(src.begin(), src.end());
    assert_equal(v.size(), 2);
    assert_equal(v[1], 5);
}

TXL_UNIT_TEST(assign_init_list)
{
    txl::fixed_vector<int, 5> v;
    v.assign({7,8,9});
    assert_equal(v.size(), 3);
    assert_equal(v[0], 7);
}

TXL_UNIT_TEST(at_throws)
{
    txl::fixed_vector<int, 5> v(2, 1);
    assert_no_throw([&]() {
        v.at(1);
    });
    assert_throws<std::out_of_range>([&]() { v.at(2); });
}

TXL_UNIT_TEST(assign_element)
{
    txl::fixed_vector<int, 5> v(2, 3);
    assert_equal(v[0], 3);
    v[1] = 10;
    assert_equal(v[1], 10);
}

TXL_UNIT_TEST(front_and_back)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    assert_equal(v.front(), 1);
    assert_equal(v.back(), 3);
}

TXL_UNIT_TEST(data)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    auto p = static_cast<int const *>(v.data());
    assert_equal(p[0], 1);
    assert_equal(p[2], 3);
}

TXL_UNIT_TEST(iterators)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    auto it = v.begin();
    assert_equal(*it, 1);
    ++it;
    assert_equal(*it, 2);
    assert_equal(std::distance(v.begin(), v.end()), 3);
}

TXL_UNIT_TEST(reverse_iterators)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    auto rit = v.rbegin();
    assert_equal(*rit, 3);
    ++rit;
    assert_equal(*rit, 2);
}

TXL_UNIT_TEST(capacity)
{
    txl::fixed_vector<int, 5> v;
    assert_true(v.empty());
    assert_equal(v.size(), 0);
    assert_equal(v.capacity(), 5);
    assert_equal(v.max_size(), 5);
}

TXL_UNIT_TEST(clear)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    v.clear();
    assert_equal(v.size(), 0);
    assert_true(v.empty());
}

TXL_UNIT_TEST(pop_back)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    v.pop_back();
    assert_equal(v.size(), 2);
    assert_equal(v.back(), 2);
}

TXL_UNIT_TEST(insert_one)
{
    txl::fixed_vector<int, 5> v{1,3};
    v.insert(v.begin() + 1, 2);
    assert_equal(v[1], 2);
    assert_equal(v.size(), 3);
}

TXL_UNIT_TEST(insert_multiple)
{
    txl::fixed_vector<int, 5> v{1,4};
    v.insert(v.begin() + 1, 2, 2);
    assert_equal(v[1], 2);
    assert_equal(v[2], 2);
    assert_equal(v[3], 4);
}

TXL_UNIT_TEST(insert_range)
{
    std::vector<int> src = {2,3};
    txl::fixed_vector<int, 5> v{1,4};
    v.insert(v.begin() + 1, src.begin(), src.end());
    assert_equal(v[1], 2);
    assert_equal(v[2], 3);
    assert_equal(v[3], 4);
}

TXL_UNIT_TEST(insert_init_list)
{
    txl::fixed_vector<int, 5> v{1,5};
    v.insert(v.begin() + 1, {2,3,4});
    assert_equal(v[1], 2);
    assert_equal(v[3], 4);
    assert_equal(v[4], 5);
}

TXL_UNIT_TEST(emplace)
{
    txl::fixed_vector<std::string, 5> v{"a", "c"};
    v.emplace(v.begin() + 1, 1, 'b');
    assert_equal(v[1], "b");
}

TXL_UNIT_TEST(erase_one)
{
    txl::fixed_vector<int, 5> v{1,2,3};
    v.erase(v.begin() + 1);
    assert_equal(v.size(), 2);
    assert_equal(v[1], 3);
}

TXL_UNIT_TEST(erase_range)
{
    txl::fixed_vector<int, 5> v{1,2,3,4};
    v.erase(v.begin() + 1, v.begin() + 3);
    assert_equal(v.size(), 2);
    assert_equal(v[1], 4);
}

TXL_UNIT_TEST(swap)
{
    txl::fixed_vector<int, 5> v1{1,2};
    txl::fixed_vector<int, 5> v2{3,4,5};
    v1.swap(v2);
    assert_equal(v1.size(), 3);
    assert_equal(v2.size(), 2);
    assert_equal(v1[0], 3);
    assert_equal(v2[1], 2);
}

TXL_UNIT_TEST(equality)
{
    txl::fixed_vector<int, 5> v1{1,2,3};
    txl::fixed_vector<int, 5> v2{1,2,3};
    assert_true(v1 == v2);
    v2[2] = 4;
    assert_false(v1 == v2);
}

TXL_UNIT_TEST(inequality)
{
    txl::fixed_vector<int, 5> v1{1,2};
    txl::fixed_vector<int, 5> v2{1,3};
    assert_true(v1 != v2);
}

TXL_UNIT_TEST(less_than)
{
    txl::fixed_vector<int, 5> v1{1,2};
    txl::fixed_vector<int, 5> v2{1,3};
    assert_true(v1 < v2);
}

TXL_UNIT_TEST(less_greater_equal)
{
    txl::fixed_vector<int, 5> v1{1,2};
    txl::fixed_vector<int, 5> v2{1,2};
    txl::fixed_vector<int, 5> v3{1,3};
    assert_true(v1 <= v2);
    assert_true(v3 > v2);
    assert_true(v3 >= v2);
}


TXL_RUN_TESTS()
