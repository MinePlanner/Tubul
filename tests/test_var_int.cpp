//
// Created by Carlos Acosta on 20-04-23.
//
#include "tubul.h"
#include <gtest/gtest.h>




TEST(TUBULVariableIntRepr, testCountLeadingZeroes) {
    using TU::helper::clz;

    EXPECT_EQ(31, clz(0x1U)); //1
    EXPECT_EQ(30, clz(0x2U)); //2
    EXPECT_EQ(25, clz(0x7FU));//127
    EXPECT_EQ(24, clz(0x80U));//128
    EXPECT_EQ(24, clz(0x8FU));//143
#ifndef TUBUL_WINDOWS
    EXPECT_EQ(63, clz(0x1UL));
    EXPECT_EQ(62, clz(0x2UL));
    EXPECT_EQ(57, clz(0x7FUL));//127
    EXPECT_EQ(56, clz(0x80UL));//128
    EXPECT_EQ(56, clz(0x8FUL));//143
#endif
    //All my tests have showed that unsigned long long is just
    //the same as unsigned long. I added tests just in case we have to
    //work on an architecture where this actually matters so we notice
    //the change.
    EXPECT_EQ(63, clz(0x1ULL));
    EXPECT_EQ(62, clz(0x2ULL));
    EXPECT_EQ(57, clz(0x7FULL));
    EXPECT_EQ(56, clz(0x80ULL));
    EXPECT_EQ(56, clz(0x8FULL));
}

TEST(TUBULVariableIntRepr, testCountBytes) {
    using TU::helper::bytesUsed;

    EXPECT_EQ(1, bytesUsed(0x1U)); //1
    EXPECT_EQ(1, bytesUsed(0xFU)); //15
    EXPECT_EQ(1, bytesUsed(0xFFU)); //255
    EXPECT_EQ(2, bytesUsed(0x100U));//256
    EXPECT_EQ(2, bytesUsed(0xFFFU)); //4095
    EXPECT_EQ(2, bytesUsed(0x1000U)); //4096
    EXPECT_EQ(3, bytesUsed(0xFFFFFU)); //65535
    EXPECT_EQ(3, bytesUsed(0x10000U)); //65536
    EXPECT_EQ(3, bytesUsed(0x100000U)); //1048576
    EXPECT_EQ(3, bytesUsed(0xFFFFFFU)); //16777215
    EXPECT_EQ(4, bytesUsed(0x1000000U)); //16777216

    uint32_t ival = 127; uint64_t lval = 127;
    EXPECT_EQ(bytesUsed(ival), bytesUsed(lval)); //1
    EXPECT_EQ(1, bytesUsed(lval)); //1
    ival = 255; lval = 255;
    EXPECT_EQ(bytesUsed(ival), bytesUsed(lval)); //1
    EXPECT_EQ(1, bytesUsed(lval)); //1
    ival = 256; lval = 256;
    EXPECT_EQ(bytesUsed(ival), bytesUsed(lval)); //2
    EXPECT_EQ(2, bytesUsed(lval)); //2
}

TEST(TUBULVariableIntRepr, testEnconding) {
    using TU::bytesAsVarint;
    using TU::toVarint;
    using TU::fromVarint;

    auto round_trip = [](uint64_t x, size_t expected_length){
        auto eb = bytesAsVarint(x);
        if (x != 0)
            EXPECT_EQ(eb, expected_length);
        auto encoded = toVarint(x);
        EXPECT_EQ(encoded.size(), expected_length);
        auto decoded = fromVarint(encoded);
        EXPECT_EQ(decoded, x);
    };
    round_trip(0,1);
    round_trip(1,1);
    round_trip(127,1);
    round_trip(128,2);
    round_trip(255,2);
    round_trip(16384,3);
    round_trip( 2097151,3);
    round_trip( 2097152,4);
    round_trip( 4194302,4);
    round_trip(3679899543542109203,9);
}