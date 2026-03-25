#include <txl/unit_test.h>
#include <txl/feature_flag.h>
#include <sstream>
#include <string>

class feature_broker
{
protected:
    auto default_feature(std::string const & name, int num) -> std::string
    {
        std::ostringstream ss;
        ss << "Your name is " << name << " and your lucky number is " << num;
        return ss.str();
    }

    auto new_feature(std::string const & name, int num) -> std::string
    {
        std::ostringstream ss;
        ss << "Name: " << name << "; Lucky Number: " << num;
        return ss.str();
    }
    
    auto default_void_feature(int num) -> void
    {
        void_feature_called_ = 1;
        num_ = num * 2;
    }

    auto new_void_feature(int num) -> void
    {
        void_feature_called_ = 2;
        num_ = num;
    }
private:
    FUNCTION_LIKE(feature_broker::default_feature) curr_;
    FUNCTION_LIKE(feature_broker::default_void_feature) curr_v_;
    int void_feature_called_ = 0;
    int num_ = 0;
public:
    feature_broker()
        : curr_{CLASS_METHOD(default_feature)}
        , curr_v_{CLASS_METHOD(default_void_feature)}
    {
    }

    auto set_version(int ver) -> void
    {
        if (ver == 2)
        {
            curr_ = CLASS_METHOD(new_feature);
            curr_v_ = CLASS_METHOD(new_void_feature);
        }
        else
        {
            curr_ = CLASS_METHOD(default_feature);
            curr_v_ = CLASS_METHOD(default_void_feature);
        }
    }

    auto num() const -> int { return num_; }
    auto void_feature_called() const -> int { return void_feature_called_; }

    BIND_FEATURE(curr_, invoke_feature);
    BIND_FEATURE(curr_v_, invoke_void_feature);
};

TXL_UNIT_TEST(feature_flag)
{
    feature_broker fb{};
    auto x = fb.invoke_feature("Red", 42);
    assert_equal(x, "Your name is Red and your lucky number is 42");

    fb.set_version(2);
    x = fb.invoke_feature("Red", 42);
    assert_equal(x, "Name: Red; Lucky Number: 42");
}

TXL_UNIT_TEST(void_feature_flag)
{
    feature_broker fb{};
    fb.invoke_void_feature(27);
    assert_equal(fb.num(), 27*2);
    assert_equal(fb.void_feature_called(), 1);

    fb.set_version(2);
    fb.invoke_void_feature(27);
    assert_equal(fb.num(), 27);
    assert_equal(fb.void_feature_called(), 2);
}

TXL_RUN_TESTS()
