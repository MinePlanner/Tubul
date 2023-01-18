//
// Created by Carlos Acosta on 12-01-23.
//

#include <gtest/gtest.h>
#include <algorithm>
#include "tubul.h"

TEST(TUBULString, testSplitEmpty){
	std::vector<std::string> tests = {""," ", "  " } ;
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string );
		EXPECT_EQ(result.size(), 0);
		//Because we just found delimers, there's nothing "real" to cheack
	}
}

TEST(TUBULString, testSplit1){

	std::vector<std::string> tests = {"P"," P", "P  ","    P    "  } ;
	std::string expected = "P";
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string );
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
		auto result = TU::split(test_string);
		EXPECT_EQ(result.size(), expected.size());
		EXPECT_EQ(result, result);
	}
}

TEST(TUBULString, testSplitStrictEmpty){

	std::vector<std::string_view> result;
	std::vector<std::string> tests = {"", ",", ",,", ",,," } ;
	size_t expected_size = 1;
	for ( auto const& test_string: tests)
	{
		result = TU::split(test_string,"," );
		EXPECT_EQ(result.size(), expected_size);
		for (auto const& it: result)
			EXPECT_EQ(it,"");
		expected_size++;
	}
}

TEST(TUBULString, testSplitStrictReal){
	std::string test_str = "Mafalda";
	auto result = TU::split(test_str,",");
	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result.front(), test_str.c_str());

	//The first vector contains different sets of strings separated
	//by commas as we could find in a CSV. The second contains
	//the corresponding expected ersult
	std::vector<std::string> tests = {"Mafalda,Felipito",
									  "Mafalda,Felipito,Miguelito",
									  "Mafalda,,Miguelito",
									  "Mafalda,Felipito,",
									  ",Felipito,Miguelito",
									  "Mafalda,Felipito,Miguelito,Susanita",
									  ",Felipito,Miguelito,",
									  "Mafalda,,,Susanita",
										} ;
	std::vector<std::vector<std::string>> expected = { {"Mafalda","Felipito"},
													  {"Mafalda","Felipito","Miguelito"},
													   {"Mafalda","","Miguelito"},
													   {"Mafalda","Felipito",""},
													   {"","Felipito","Miguelito"},
													  {"Mafalda","Felipito","Miguelito","Susanita"},
													   {"","Felipito","Miguelito",""},
													   {"Mafalda","","","Susanita"},
													};
	size_t current_test = 0;
	for ( auto const& test_string: tests)
	{
		result = TU::split(test_string,"," );
		auto const& current_expected = expected[current_test];
		EXPECT_EQ(result.size(),current_expected.size()) ;
		bool was_equal = std::equal(result.begin(), result.end(), expected[current_test].begin());
		EXPECT_EQ(was_equal,true ) ;
		current_test++;
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
