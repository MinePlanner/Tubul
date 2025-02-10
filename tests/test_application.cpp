#include <gtest/gtest.h>
#include "tubul.h"

TEST(TUBULAPP, testAppInit){
	std::string name = "TEST_APP";
	std::string version = "0.3";

	TU::initTubulApp(name, version);

	EXPECT_EQ(name, TU::getAppName());
	EXPECT_EQ(version, TU::getAppVer());
}