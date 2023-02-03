
#include <gtest/gtest.h>
#include "tubul.h"

TEST(TUBULSetup, testInit){
    int version = TU::getVersion();
    EXPECT_EQ(version, 0);
}


