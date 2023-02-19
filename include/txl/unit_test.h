#pragma once

#include <list>
#include <iostream>
#include <cassert>

#define TXL_UNIT_TEST(name) struct test_##name : txl::unit_test  \
    {   \
        test_##name() = default;    \
        test_##name(txl::init_test_t)    \
        {   \
            txl::add_test(this); \
        }   \
        \
        void begin_test() override  \
        {   \
            std::cout << "[test_" << #name << "] -- "; \
        }   \
        void test() override;   \
    };  \
    static test_##name test##__LINE__{txl::init_test};   \
    void test_##name::test()

#define TXL_RUN_TESTS() int main()  \
    {   \
        txl::run_tests();   \
    }

namespace txl
{
    struct init_test_t {};
    static init_test_t init_test{};

    struct unit_test
    {
        virtual void begin_test() = 0;
        virtual void end_test()
        {
            std::cout << "passed" << std::endl;
        }
        virtual void test() = 0;
    };

    static std::list<unit_test *> _tests;

    static void add_test(unit_test * test)
    {
        _tests.emplace_back(test);
    }

    static void run_tests()
    {
        for (auto & test : _tests)
        {
            test->begin_test();
            test->test();
            test->end_test();
        }
    }
}
