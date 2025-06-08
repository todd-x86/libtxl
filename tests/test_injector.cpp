#include <txl/unit_test.h>
#include <txl/injector.h>

struct weak_interface
{
    virtual auto execute() -> void = 0;
    virtual auto is_executed() const -> bool = 0;
};

struct impl : weak_interface
{
    int & num_executed_;
    static int num_created;

    impl(int & num_executed)
        : num_executed_{num_executed}
    {
        ++num_created;
        num_executed_ = 0;
    }

    auto execute() -> void override
    {
        ++num_executed_;
    }
    auto is_executed() const -> bool override
    {
        return num_executed_ > 0;
    }
};

int impl::num_created = 0;

TXL_UNIT_TEST(injector)
{
    impl::num_created = 0;
    int num_execs = 0;

    txl::injector i{};
    i.factory<int>([](auto &) { return new int{42}; });
    i.factory<weak_interface, impl>([&](auto &) { return new impl(num_execs); });

    assert_equal(impl::num_created, 0);

    auto num = i.get<int>();
    assert_not_equal(num, nullptr);
    assert_equal(*num, 42);

    {
        auto w = i.get<weak_interface>();
        assert_equal(impl::num_created, 1);
        assert_not_equal(w, nullptr);
        assert_false(w->is_executed());
        assert_equal(num_execs, 0);
        w->execute();
        assert_equal(num_execs, 1);
        assert_true(w->is_executed());
    }
    {
        auto w = i.get<weak_interface>();
        assert_equal(impl::num_created, 1);
        assert_not_equal(w, nullptr);
        w->execute();
        assert_equal(num_execs, 2);
        assert_true(w->is_executed());
    }
    {
        auto w = i.get<impl>();
        assert_equal(impl::num_created, 1);
        assert_not_equal(w, nullptr);
        w->execute();
        assert_equal(num_execs, 3);
        assert_true(w->is_executed());
    }
}

TXL_RUN_TESTS()
