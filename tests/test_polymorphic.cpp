#include <txl/unit_test.h>
#include <txl/polymorphic.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

template<class T>
using string_map = std::map<T, std::string>;

static const std::string TEST_STR1 = "I can store strings",
                         TEST_STR2 = "I'm a string vector",
                         TEST_STR3 = "Just more strings",
                         TEST_STR4 = "Coming up with good examples is hard";

static size_t num_widgets_created = 0,
              num_widgets_destroyed = 0;

struct widget final
{
    int x, y, width, height;

    widget(int x, int y, int w, int h)
        : x{x}, y{y}, width{w}, height{h}
    {
        ++num_widgets_created;
    }

    ~widget()
    {
        ++num_widgets_destroyed;
    }
};

TXL_UNIT_TEST(polymorphic_map)
{
    txl::polymorphic_map map{};
    auto & w = map.get<widget>(1,1,2,3);
    assert_equal(w.x, 1);
    assert_equal(w.y, 1);
    assert_equal(w.width, 2);
    assert_equal(w.height, 3);
    map.get<std::string>() = "Hello world";
    assert_equal(map.get<std::string>(), "Hello world");
    
    map.get<int>(100);
    assert_equal(map.get<int>(), 100);

    assert_equal(map.size(), 3);

    assert_true(map.contains(txl::get_type_info<int>()));
    assert_true(map.contains(txl::get_type_info<std::string>()));
    assert_true(map.contains(txl::get_type_info<widget>()));
    assert_false(map.contains(txl::get_type_info<double>()));
    
    assert_true(map.contains<int>());
    assert_true(map.contains<std::string>());
    assert_true(map.contains<widget>());
    assert_false(map.contains<double>());
}

TXL_UNIT_TEST(polymorphic_container_map)
{
    txl::polymorphic_container_map<std::vector> bunch_o_vectors{};

    {
        auto & string_vec = bunch_o_vectors.get<std::string>();
        string_vec.emplace_back(TEST_STR1);
        string_vec.emplace_back(TEST_STR2);
        string_vec.emplace_back(TEST_STR3);
        string_vec.emplace_back(TEST_STR4);
    }

    {
        auto & double_vec = bunch_o_vectors.get<double>();
        double_vec.push_back(1.0);
        double_vec.push_back(1.1);
        double_vec.push_back(2.3);
        double_vec.push_back(4.56);
        double_vec.push_back(7.8);
    }

    assert_equal(bunch_o_vectors.get<std::string>().size(), 4);
    assert_equal(bunch_o_vectors.get<double>().size(), 5);
    assert_equal(bunch_o_vectors.get<std::string>(), std::vector<std::string>{TEST_STR1, TEST_STR2, TEST_STR3, TEST_STR4});
    assert_equal(bunch_o_vectors.get<double>(), std::vector<double>{1.0,1.1,2.3,4.56,7.8});
}

TXL_UNIT_TEST(polymorphic_container_map_string_maps)
{
    txl::polymorphic_container_map<string_map> map_to_string{};
    {
        auto & int_to_str = map_to_string.get<int>();
        int_to_str[1] = "One";
        int_to_str[2] = "Two";
        int_to_str[3] = "Third";
    }
    {
        auto & str_to_str = map_to_string.get<std::string>();
        str_to_str["A"] = "Apple";
        str_to_str["B"] = "Banana";
        str_to_str["C"] = "Canteloupe";
    }

    assert_equal(map_to_string.get<int>()[1], "One");
    assert_equal(map_to_string.get<std::string>()["A"], "Apple");
}

template<class T>
using ptr_vector = std::vector<std::unique_ptr<T>>;

TXL_UNIT_TEST(polymorphic_container_map_cleanup)
{
    num_widgets_created = num_widgets_destroyed = 0;
    {
        // using unique_ptr to better track lifetimes
        txl::polymorphic_container_map<ptr_vector> map_of_one_thing{};
        assert_equal(num_widgets_created, 0);
        assert_equal(num_widgets_destroyed, 0);

        map_of_one_thing.get<widget>().emplace_back(std::make_unique<widget>(0,0,10,10));
        assert_equal(num_widgets_created, 1);
        assert_equal(num_widgets_destroyed, 0);
        map_of_one_thing.get<widget>().emplace_back(std::make_unique<widget>(10,10,10,10));
        assert_equal(num_widgets_created, 2);
        assert_equal(num_widgets_destroyed, 0);
        map_of_one_thing.get<widget>().emplace_back(std::make_unique<widget>(20,20,10,10));

        assert_equal(num_widgets_created, 3);
        assert_equal(num_widgets_destroyed, 0);
    }
    assert_equal(num_widgets_created, 3);
    assert_equal(num_widgets_destroyed, 3);
}

TXL_RUN_TESTS()
