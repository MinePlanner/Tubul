//
// Created by Carlos Acosta on 12-01-23.
//

#include <gtest/gtest.h>
#include "tubul.h"

TEST(TUBULString, testSplitEmpty){
	std::vector<std::string> tests = {""," ", "  " } ;
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string, " ");
		EXPECT_EQ(result.size(), 0);
		//Because we just found delimers, there's nothing "real" to cheack
	}
}
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

TEST(TUBULString, testJoinEmpty) {

	std::vector<std::string> test;
	std::string res;
	// 100% empty test
	res = TU::join(test, "");
	EXPECT_EQ(res, "");

	// Single empty string
	test = {""};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "");

	// N empty string
	test = {"", "",""};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "");

	//Empty strings, but an actual joiner (like for csv)
	test = {"", "",""};
	res  = TU::join(test, ",");
	EXPECT_EQ(res, ",,");
}

TEST(TUBULString, testJoinNoJoiner)
{
	std::vector<std::string> test;
	std::string res;
	//Simple case
	test = {"A"};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "A");

	//Just concatenating because the joiner is empty
	test = {"A", "B"};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "AB");

	test = {"A", "B", "C"};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "ABC");
}

TEST(TUBULString, testJoin){

	std::vector<std::string> test;
	std::string res;
	//Checking we can actually join 2 strings with comma
	test = {"A","B"};
	res = TU::join(test, ",");
	EXPECT_EQ(res, "A,B");

	//Checking we can actually join 2 strings with a random string
	test = {"A","B"};
	res = TU::join(test, "->");
	EXPECT_EQ(res, "A->B");

	test = {"A","B"};
	res = TU::join(test, ",");
	EXPECT_EQ(res, "A,B");

	test = {"A","B"};
	res = TU::join(test, "->");
	EXPECT_EQ(res, "A->B");
}
