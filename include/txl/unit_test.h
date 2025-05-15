#pragma once

#include <txl/type_info.h>

#include <chrono>
#include <list>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#define _TXL_TEST_NAME(n) __test_##n
#define _TXL_VARIATION_NAME(n) __test_variation_##n

#define _TXL_UNIT_TEST(name, iters, max_time) struct _test_##name : txl::unit_test  \
    {   \
        _test_##name() = default;    \
        _test_##name(txl::init_test_t)    \
        {   \
            txl::add_test(this, iters, max_time); \
        }   \
        \
        void _begin_test(std::string_view variation) override  \
        {   \
            if (variation.empty())  \
            {   \
                std::cout << "[test_" << #name << "] -- "; \
            }   \
            else    \
            {   \
                std::cout << "[test_" << #name << " (" << variation << ")] -- "; \
            }   \
        }   \
        void _test() override;   \
    };  \
    static _test_##name _TXL_TEST_NAME(name){txl::init_test};   \
    void _test_##name::_test()

#define TXL_UNIT_TEST_N(name, iters) _TXL_UNIT_TEST(name, iters, std::nullopt)
#define TXL_UNIT_TEST(name) _TXL_UNIT_TEST(name, 1, std::nullopt)
#define TXL_UNIT_TEST_TIME(name, time) _TXL_UNIT_TEST(name, 1, time)

#define TXL_UNIT_TEST_VARIATION(name, func) struct _test_variation_##name : txl::unit_test_variation  \
    {   \
        _test_variation_##name() = default; \
        _test_variation_##name(txl::init_test_t)    \
        {   \
            txl::add_test_variation(this);   \
        }   \
        \
        auto get_name() const -> std::string_view override  \
        {   \
            return #name;   \
        }   \
        \
        auto setup() -> void override   \
        {   \
            func(); \
        }   \
    };  \
    static _test_variation_##name _TXL_VARIATION_NAME(name){txl::init_test}

#define TXL_RUN_TESTS() int main()  \
    {   \
        return txl::run_tests();   \
    }

namespace txl
{
    struct init_test_t {};
    static init_test_t init_test{};

    struct unit_test_variation
    {
        virtual auto setup() -> void = 0;
        virtual auto get_name() const -> std::string_view = 0;
    };

    struct unit_test
    {
        template<class T>
        struct test_printer final
        {
            T const & value_;

            test_printer(T const & value)
                : value_{value}
            {
            }
        };

        std::ostringstream error_buf_;

        struct assertion_error final : std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        virtual void _begin_test(std::string_view variation = {}) = 0;
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

        auto assert(bool value, std::string_view custom_message = "assert") -> void
        {
            assert_equal(true, value, custom_message);
        }

        auto assert_true(bool value, std::string_view custom_message = "assert_true") -> void
        {
            assert_equal(true, value, custom_message);
        }

        auto assert_false(bool value, std::string_view custom_message = "assert_false") -> void
        {
            assert_equal(false, value, custom_message);
        }
        
        template<class ExpectedValue, class ActualValue>
        auto assert_less_than(ActualValue const & actual, ExpectedValue const & expected, std::string_view custom_message = "assert_less_than") -> void
        {
            if (not (static_cast<ExpectedValue const &>(actual) < expected))
            {
                error_buf_ << custom_message << ": " << test_printer<ActualValue>{actual} << " (actual) >= " << test_printer<ExpectedValue>{expected} << " (expected)";
                throw assertion_error("assertion failed");
            }
        }
        
        template<class ExpectedValue, class ActualValue>
        auto assert_less_than_equal(ActualValue const & actual, ExpectedValue const & expected, std::string_view custom_message = "assert_less_than_equal") -> void
        {
            if (not (static_cast<ExpectedValue const &>(actual) <= expected))
            {
                error_buf_ << custom_message << ": " << test_printer<ActualValue>{actual} << " (actual) > " << test_printer<ExpectedValue>{expected} << " (expected)";
                throw assertion_error("assertion failed");
            }
        }
        
        template<class ExpectedValue, class ActualValue>
        auto assert_greater_than(ActualValue const & actual, ExpectedValue const & expected, std::string_view custom_message = "assert_greater_than") -> void
        {
            if (not (static_cast<ExpectedValue const &>(actual) > expected))
            {
                error_buf_ << custom_message << ": " << test_printer<ActualValue>{actual} << " (actual) <= " << test_printer<ExpectedValue>{expected} << " (expected)";
                throw assertion_error("assertion failed");
            }
        }
        
        template<class ExpectedValue, class ActualValue>
        auto assert_greater_than_equal(ActualValue const & actual, ExpectedValue const & expected, std::string_view custom_message = "assert_greater_than_equal") -> void
        {
            if (not (static_cast<ExpectedValue const &>(actual) >= expected))
            {
                error_buf_ << custom_message << ": " << test_printer<ActualValue>{actual} << " (actual) < " << test_printer<ExpectedValue>{expected} << " (expected)";
                throw assertion_error("assertion failed");
            }
        }

        template<class ExpectedValue, class ActualValue>
        auto assert_equal(ExpectedValue const & expected, ActualValue const & actual, std::string_view custom_message = "assert_equal") -> void
        {
            if (not (expected == static_cast<ExpectedValue const &>(actual)))
            {
                error_buf_ << custom_message << ": " << test_printer<ExpectedValue>{expected} << " (expected) != " << test_printer<ActualValue>{actual} << " (actual)";
                throw assertion_error("assertion failed");
            }
        }
        
        template<class ExpectedValue, class ActualValue>
        auto assert_not_equal(ExpectedValue const & expected, ActualValue const & actual, std::string_view custom_message = "assert_not_equal") -> void
        {
            if ((expected == static_cast<ExpectedValue const &>(actual)))
            {
                error_buf_ << custom_message << ": " << test_printer<ExpectedValue>{expected} << " (expected) == " << test_printer<ActualValue>{actual} << " (actual)";
                throw assertion_error("assertion failed");
            }
        }

        template<class Exception, class Func>
        auto assert_throws(Func && func, std::string_view custom_message = "assert_throws") -> void
        {
            try
            {
                func();

                error_buf_ << custom_message << ": no exception thrown";
                throw assertion_error{"assertion failed"};
            }
            catch (Exception const & ex)
            {
                return;
            }
            catch (...)
            {
                error_buf_ << custom_message << ": different exception thrown";
                throw assertion_error{"assertion failed"};
            }
        }
        
        template<class Func>
        auto assert_no_throw(Func && func, std::string_view custom_message = "assert_no_throw") -> void
        {
            try
            {
                func();
            }
            catch (...)
            {
                error_buf_ << custom_message << ": exception thrown";
                throw assertion_error{"assertion failed"};
            }
        }

        auto _set_error(std::string_view msg) -> void
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

        struct unit_test_data final
        {
            unit_test * test_body;
            size_t num_loops = 1;
            std::optional<std::chrono::nanoseconds> max_time;

            unit_test_data(unit_test * t, size_t n, std::optional<std::chrono::nanoseconds> mt)
                : test_body{t}
                , num_loops{n}
                , max_time{mt}
            {
            }
        };
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
    
    template<class T1, class T2>
    inline auto operator<<(std::ostream & os, unit_test::test_printer<std::pair<T1, T2>> const & value) -> std::ostream &
    {
        os << "{" << unit_test::test_printer<T1>{value.value_.first} << ", " << unit_test::test_printer<T2>{value.value_.second} << "}";
        return os;
    }
    
    inline auto operator<<(std::ostream & os, unit_test::test_printer<bool> const & value) -> std::ostream &
    {
        os << std::boolalpha << value.value_;
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

    static std::list<detail::unit_test_data> __tests;
    static std::list<unit_test_variation *> __variations;

    static void add_test(unit_test * test, size_t num_loops = 1, std::optional<std::chrono::nanoseconds> max_time = std::nullopt)
    {
        __tests.emplace_back(test, num_loops, max_time);
    }
    
    [[maybe_unused]] static void add_test_variation(unit_test_variation * variation)
    {
        __variations.emplace_back(variation);
    }

    static auto run_all_tests(std::string_view variation_name = {}) -> bool
    {
        auto success = true;
        for (auto & test_data : __tests)
        {
            auto test = test_data.test_body;

            test->_begin_test(variation_name);
            try
            {
                if (test_data.max_time)
                {
                    auto test_duration = *test_data.max_time;
                    for (size_t i = 0; i < test_data.num_loops; ++i)
                    {
                        auto start_time = std::chrono::high_resolution_clock::now();
                        test->_test();
                        auto end_time = std::chrono::high_resolution_clock::now();

                        // Check duration of time spent
                        test->assert_less_than_equal(end_time-start_time, test_duration, "test duration exceeded");
                    }
                }
                else
                {
                    for (size_t i = 0; i < test_data.num_loops; ++i)
                    {
                        test->_test();
                    }
                }
                test->_end_test(true);
            }
            catch (unit_test::assertion_error const & err)
            {
                success = false;
                test->_end_test(false);
            }
            catch (std::exception const & ex)
            {
                success = false;
                test->_set_error(ex.what());
                test->_end_test(false);
            }
            catch (...)
            {
                success = false;
                test->_set_error("unknown exception type thrown");
                test->_end_test(false);
            }
        }
        return success;
    }

    static auto run_tests() -> int
    {
        auto exit_code = 0;
        if (__variations.empty())
        {
            if (not run_all_tests())
            {
                exit_code = 1;
            }
        }
        else
        {
            for (auto & v : __variations)
            {
                v->setup();
                if (not run_all_tests(v->get_name()))
                {
                    exit_code = 1;
                }
            }
        }

        return exit_code;
    }
}
