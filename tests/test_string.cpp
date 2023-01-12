//
// Created by Carlos Acosta on 12-01-23.
//

#include <gtest/gtest.h>
#include "tubul.h"

TEST(TUBULString, testSplit1){

	std::vector<std::string> tests = {"P"," P", "P  ","    P    "  } ;
	std::string expected = "P";
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string, " ");
		EXPECT_EQ(result.size(), 1);
		EXPECT_EQ(result.front(), expected);
	}
}

TEST(TUBULString, testSplit2){

	std::vector<std::string> tests = {
		"Pedrito clavo un clavito",
		"  Pedrito clavo un clavito",
		"Pedrito clavo un clavito   ",
		"     Pedrito clavo un clavito   ",
		"     Pedrito    clavo  un   clavito   "
	} ;
	std::vector<std::string> expected = {"Pedrito","clavo", "un", "clavito"};
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string, " ");
		EXPECT_EQ(result.size(), expected.size());
		EXPECT_EQ(result, result);
	}
}