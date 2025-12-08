#include <txl/unit_test.h>
#include <txl/vector.h>

TXL_UNIT_TEST(vector_find)
{
    txl::vector<std::string> names{"Sam Lowry", "Ida Lowry", "Eugene Helpmann", "Harvey Lime", "Jack Lint"};
    assert_not_equal(names.find("Ida Lowry"), names.end());
    assert_equal(names.find("Harry Tuttle"), names.end());
    assert_equal(*names.find("Sam Lowry"), "Sam Lowry");
}

TXL_UNIT_TEST(vector_index_of)
{
    txl::vector<std::string> names{"Walter White", "Jesse Pinkman", "Saul Goodman"};
    assert_equal(*names.index_of("Jesse Pinkman"), 1);
    assert_equal(names.index_of("Hank Schrader"), std::nullopt);
}

TXL_UNIT_TEST(vector_contains)
{
    txl::vector<std::string> names{"Mr. Bean", "Irma Gobb", "Teddy"};
    assert_true(names.contains("Teddy"));
    assert_false(names.contains("Johnny English"));
}

TXL_UNIT_TEST(vector_erase_at)
{
    txl::vector<std::string> names{"Michael Scott", "Jim Halpert", "Pam Beesly", "Kevin Malone"};
    assert_true(names.contains("Kevin Malone"));
    names.erase_at(3);
    assert_false(names.contains("Kevin Malone"));
}

TXL_UNIT_TEST(vector_sort)
{
    txl::vector<std::string> names{"Buzz", "Woody", "Rex", "Andy", "Sarge", "Slinky", "Mr. Potato Head"};
    names.sort();
    assert_equal(names, txl::vector<std::string>{"Andy", "Buzz", "Mr. Potato Head", "Rex", "Sarge", "Slinky", "Woody"});
}

TXL_RUN_TESTS()
