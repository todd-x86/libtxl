#include <txl/unit_test.h>
#include <txl/one_of.h>

enum class mood
{
    happy,
    sad,
    apathetic,
    angry,
    ecstatic,
    sleeping,
};

TXL_UNIT_TEST(one_of)
{
    auto my_mood = mood::sleeping;
    auto is_happy = txl::one_of(my_mood, mood::happy, mood::ecstatic);
    auto is_unhappy = txl::one_of(my_mood, mood::sad, mood::apathetic, mood::angry);

    assert(not is_happy and not is_unhappy);
}

TXL_RUN_TESTS()
