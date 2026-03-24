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
private:
    FUNCTION_LIKE(feature_broker::default_feature) curr_;
public:
    feature_broker()
        : curr_{CLASS_METHOD(default_feature)}
    {
    }

    auto set_version(int ver) -> void
    {
        if (ver == 2)
        {
            curr_ = CLASS_METHOD(new_feature);
        }
        else
        {
            curr_ = CLASS_METHOD(default_feature);
        }
    }

    BIND_FEATURE(curr_, invoke_feature);
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

TXL_RUN_TESTS()
