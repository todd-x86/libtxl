#pragma once

#include <list>
#include <iostream>
#include <cassert>

#define _TXL_TEST_NAME(n) test_##n

#define TXL_UNIT_TEST(name) struct _test_##name : txl::unit_test  \
    {   \
        _test_##name() = default;    \
        _test_##name(txl::init_test_t)    \
        {   \
            txl::add_test(this); \
        }   \
        \
        void _begin_test() override  \
        {   \
            std::cout << "[test_" << #name << "] -- "; \
        }   \
        void _test() override;   \
    };  \
    static _test_##name _TXL_TEST_NAME(name){txl::init_test};   \
    void _test_##name::_test()

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
        virtual void _begin_test() = 0;
        virtual void _end_test()
        {
            std::cout << "passed" << std::endl;
        }
        virtual void _test() = 0;
    };

    static std::list<unit_test *> __tests;

    static void add_test(unit_test * test)
    {
        __tests.emplace_back(test);
    }

    static void run_tests()
    {
        for (auto & test : __tests)
        {
            test->_begin_test();
            test->_test();
            test->_end_test();
        }
    }
}
