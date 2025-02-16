#pragma once

#include <txl/type_info.h>

#include <list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#define _TXL_TEST_NAME(n) __test_##n

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
        return txl::run_tests();   \
    }

namespace txl
{
    struct init_test_t {};
    static init_test_t init_test{};

    struct unit_test
    {
        template<class T>
        struct test_printer final
        {
            T const & value_;
        };

        std::ostringstream error_buf_;

        struct assertion_error final : std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        virtual void _begin_test() = 0;
        virtual void _end_test(bool success)
        {
            if (success)
            {
                std::cout << "passed" << std::endl;
            }
            else
            {
                std::cout << "failed" << std::endl;
                auto err = error_buf_.str();
                if (not err.empty())
                {
                    std::cout << " - ERROR: " << err << std::endl;
                    error_buf_.str("");
                }
            }
        }
        virtual void _test() = 0;

        static void assert(bool value)
        {
            if (not value)
            {
                throw assertion_error("assertion failed");
            }
        }

        auto assert_true(bool value)
        {
            assert(value);
        }

        auto assert_false(bool value)
        {
            assert(not value);
        }

        template<class ExpectedValue, class ActualValue>
        void assert_equal(ExpectedValue const & expected, ActualValue const & actual)
        {
            if (not (expected == static_cast<ExpectedValue const &>(actual)))
            {
                error_buf_ << "assert_equal: " << test_printer{expected} << " (expected) != " << test_printer{actual} << " (actual)";
                throw assertion_error("assertion failed");
            }
        }
        
        template<class ExpectedValue, class ActualValue>
        void assert_not_equal(ExpectedValue const & expected, ActualValue const & actual)
        {
            if ((expected == static_cast<ExpectedValue const &>(actual)))
            {
                error_buf_ << "assert_not_equal: " << test_printer{expected} << " (expected) == " << test_printer{actual} << " (actual)";
                throw assertion_error("assertion failed");
            }
        }

        template<class Exception, class Func>
        void assert_throws(Func && func)
        {
            try
            {
                func();

                error_buf_ << "assert_throws: no exception thrown";
                throw assertion_error{"assertion failed"};
            }
            catch (Exception const & ex)
            {
                return;
            }
            catch (...)
            {
                error_buf_ << "assert_throws: different exception thrown";
                throw assertion_error{"assertion failed"};
            }
        }

        auto _set_error(std::string_view msg)
        {
            error_buf_ << "exception thrown: " << msg;
        }
    };
    
    namespace detail
    {
        template<class S, class T>
        class is_streamable
        {
        private:
            template<class SS, class TT>
            static auto test(int) -> decltype(std::declval<SS &>() << std::declval<TT>(), std::true_type{});
            
            template<class, class>
            static auto test(...) -> std::false_type;
        public:
            static constexpr bool value = decltype(test<S, T>(0))::value;
        };

        template<class S, class T>
        inline constexpr bool is_streamable_v = is_streamable<S, T>{}.value;
    }
    
    template<class T>
    inline auto operator<<(std::ostream & os, unit_test::test_printer<std::vector<T>> const & value) -> std::ostream &
    {
        os << "{";
        auto comma = false;
        for (auto const & item : value.value_)
        {
            if (comma)
            {
                os << ", ";
            }
            os << unit_test::test_printer<T>(item);
            comma = true;
        }
        os << "}";
        return os;
    }

    template<class T>
    inline auto operator<<(std::ostream & os, unit_test::test_printer<T> const & value) -> std::ostream &
    {
        if constexpr (detail::is_streamable_v<std::ostream, T>)
        {
            os << value.value_;
        }
        else
        {
            os << get_type_info<T>().name() << '<' << static_cast<void const *>(&value.value_) << '>';
        }
        return os;
    }

    static std::list<unit_test *> __tests;

    static void add_test(unit_test * test)
    {
        __tests.emplace_back(test);
    }

    static auto run_tests() -> int
    {
        auto exit_code = 0;

        for (auto & test : __tests)
        {
            test->_begin_test();
            try
            {
                test->_test();
                test->_end_test(true);
            }
            catch (unit_test::assertion_error const & err)
            {
                exit_code = 1;
                test->_end_test(false);
            }
            catch (std::exception const & ex)
            {
                exit_code = 1;
                test->_set_error(ex.what());
                test->_end_test(false);
            }
            catch (...)
            {
                exit_code = 1;
                test->_set_error("unknown exception type thrown");
                test->_end_test(false);
            }
        }

        return exit_code;
    }
}
