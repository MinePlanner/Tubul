//
// Created by Carlos Acosta on 30-03-23.
//

#include "tubul.h"
#include <gtest/gtest.h>



TEST(TUBULFileUtils, testParseInt) {
    const char* val1 = "452";

    auto parsed1 = TU::strToInt(val1);
    EXPECT_EQ(parsed1, 452);

    std::string strval1("888");
    auto parsed2 = TU::strToInt(strval1);
    EXPECT_EQ(parsed2, 888);

    std::string_view strview(strval1);
    auto parsed3 = TU::strToInt(strview);
    EXPECT_EQ(parsed3, 888);

    EXPECT_EQ(TU::strToInt("-15"), -15);
    EXPECT_EQ(TU::strToInt("0"), 0);
    EXPECT_EQ(TU::strToInt("000000"), 0);
    EXPECT_EQ(TU::strToInt("000000"), 0);
    EXPECT_EQ(TU::strToInt("2000000000"), 2000000000);
    EXPECT_EQ(TU::strToInt("-2000000000"), -2000000000);

    //should fail because these strings have weird chars
    EXPECT_ANY_THROW( auto fail = TU::strToInt("3.14"));
    EXPECT_ANY_THROW( auto fail = TU::strToInt("314#"));
    EXPECT_ANY_THROW( auto fail = TU::strToInt(" 314#"));
    EXPECT_ANY_THROW( auto fail = TU::strToInt("31/3"));
}

TEST(TUBULFileUtils, testParseDouble) {
    const char* val1 = "452";

    auto parsed1 = TU::strToDouble(val1);
    EXPECT_EQ(parsed1, 452);

    std::string strval1("888.8");
    auto parsed2 = TU::strToDouble(strval1);
    EXPECT_EQ(parsed2, 888.8);

    std::string_view strview(strval1);
    auto parsed3 = TU::strToDouble(strview);
    EXPECT_EQ(parsed3, 888.8);


    EXPECT_EQ(TU::strToDouble("-15"), -15);
    EXPECT_EQ(TU::strToDouble("0.0"), 0);
    EXPECT_EQ(TU::strToDouble("0.5"), 0.5);
    EXPECT_EQ(TU::strToDouble("000000"), 0);
    EXPECT_EQ(TU::strToDouble("-0"), 0);
    EXPECT_EQ(TU::strToDouble("2000000000"), 2000000000);
    EXPECT_EQ(TU::strToDouble("-2000000000"), -2000000000);
    EXPECT_EQ(TU::strToDouble("3.14"), 3.14);
    EXPECT_EQ(TU::strToDouble("-3.14"), -3.14);
    EXPECT_EQ(TU::strToDouble("1.2e-5"), 1.2e-5);
    EXPECT_EQ(TU::strToDouble("-1e-9"), -1e-9);


    //should fail because these strings have weird chars
    EXPECT_ANY_THROW( auto fail = TU::strToDouble("314#"));
    EXPECT_ANY_THROW( auto fail = TU::strToDouble(" 314#@"));
    EXPECT_ANY_THROW( auto fail = TU::strToDouble("31/3"));
}
