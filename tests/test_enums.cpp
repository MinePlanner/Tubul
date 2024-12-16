
#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>
#include <set>

BETTER_ENUM(Test, uint8_t, val1, val2, val3)

TEST(TUBULEnums, CreateEnum)
{
    //Because gtest works inside a struct/namespace it's easier to add the using clause
    //and avoid all the ::
    using ::Test;

    //The enum should have these elements
    std::vector<Test> expected = {Test::val1, Test::val2, Test::val3 };
    EXPECT_EQ(expected.size(), Test::_size());

    //And we should be able to iterate them
    int i = 0 ;
    for( auto val: Test::_values()) {
        EXPECT_EQ(val, expected[i]);
        ++i;
    }

    //And convert to and from strings
    {
        const Test x = Test::val2;
        std::string x_as_str = x._to_string();
        EXPECT_EQ(x_as_str, "val2");
    }
    {
        //To compare with literal values form the enum, we have to use the "+" sign
        const Test x = Test::_from_string("val2");
        EXPECT_TRUE(x == +Test::val2);
    }
    //And convert to indices
    {
        const Test z = Test::_from_index(2);
        //Remember the trick with the plus sign.
        EXPECT_EQ(z, +Test::val3);
        const Test z2 = Test::val3;
        EXPECT_EQ(z2._to_index(), 2);
    }
    //and we can do switches
    Test s = Test::_from_string("val1");
    bool correct = false;
    switch (s) {
        case Test::val1:
            correct = true;
        break;
        case Test::val2:
        case Test::val3:
        default:
            correct = false;
            break;
    }
    EXPECT_TRUE(correct);
}

namespace BE
{
BETTER_ENUM(MyEnum, int, a, b, c, d, e)
}

TEST(TUBULEnums, Enum2) {
    //Because gtest works inside a struct/namespace it's easier to add the using clause
    //and avoid all the ::
    using ::BE::MyEnum;

    //The enum should have these elements
    std::vector<MyEnum> expected = {MyEnum::a, MyEnum::b, MyEnum::c, MyEnum::d, MyEnum::e};
    EXPECT_EQ(expected.size(), MyEnum::_size());

    //And we should be able to iterate them
    int i = 0 ;
    for( auto val: MyEnum::_values()) {
        EXPECT_EQ(val, expected[i]);
        ++i;
    }
}

struct ET {
    ::BE::MyEnum member_;
};

TEST(TUBULEnums, Enum3) {

    using ::BE::MyEnum;
    ET test = { MyEnum::c};
    EXPECT_EQ(test.member_, +MyEnum::c);
    //Sadly we can't use "c" directly because it would use pointer comparison :(
    EXPECT_EQ(test.member_._to_string(), std::string("c"));
    EXPECT_EQ(test.member_._to_string(), MyEnum::_names()[2]);
    EXPECT_EQ(test.member_._to_index(), 2);

}
