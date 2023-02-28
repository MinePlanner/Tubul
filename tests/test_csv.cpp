//
// Created by Carlos Acosta on 09-02-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>


const char* CSV1 = R"(name,A,B,C,D
hugo,1,2,3,4
paco,4,3,2,1
luis,3,1,2,4
)";

TEST(TUBULCSV, testBasicFunctionality)
{
	std::vector<size_t> expected   = {0, 1, 2, 3, 4};
	size_t              iter_count = 0;
	auto res = TU::readCsvFromString(CSV1);
	EXPECT_TRUE(res);
	auto const& csv_data = res.value();
	//General data peeking
	EXPECT_EQ( csv_data.colCount(),4);
	EXPECT_EQ( csv_data.rowCount(),3);

	//Checking the name of the columns
	auto const& col_names = csv_data.getColNames();
	std::vector<std::string> expected_col_names = {"A","B","C","D"};
	EXPECT_EQ(col_names.size(), expected_col_names.size());
	for (size_t i = 0; i < col_names.size(); ++i)
		EXPECT_EQ(col_names[i], expected_col_names[i]);

	//Getting a given row (the first row)
	auto hugo = csv_data.getRow(0);
	std::vector<std::string> expected_hugo = {"1","2","3","4"};
	EXPECT_EQ(hugo.size(), expected_hugo.size());
	for (size_t i = 0; i < hugo.size(); ++i )
		EXPECT_EQ(expected_hugo[i], hugo[i]);

	//Getting a given row (the last row)
	auto luis = csv_data.getRow(2);
	std::vector<std::string> expected_luis = {"3","1","2","4"};
	EXPECT_EQ(luis.size(), expected_luis.size());
	for (size_t i = 0; i < hugo.size(); ++i )
		EXPECT_EQ(expected_hugo[i], hugo[i]);

	//Get the second column (column B)
	auto colB = csv_data.getColumnAsInteger(1);
	std::vector<int> expected_colB= {2,3,1};
	EXPECT_EQ(colB.size(), expected_colB.size());
	for (size_t i = 0; i < colB.size(); ++i )
		EXPECT_EQ(expected_colB[i], colB[i]);

}
TEST(TUBULCSV, testColumnConversion)
{
	auto res = TU::readCsvFromString(CSV1);
	EXPECT_TRUE(res);

	auto& csv_file = res.value();
	auto cols = csv_file.convertAllToColumnFormat();

	//Get columns
	auto colA = std::get< TU::IntegerColumn >(cols["A"]);
	auto colB = std::get< TU::IntegerColumn >(cols["B"]);
	auto colC = std::get< TU::IntegerColumn >(cols["C"]);
	auto colD = std::get< TU::IntegerColumn >(cols["D"]);
	EXPECT_EQ(colA.size(),3);
	TU::IntegerColumn expected_col_a = {1,4,3};
	for (size_t i =0; i< colA.size(); ++i)
		EXPECT_EQ(colA[i], expected_col_a[i]);

	EXPECT_EQ(colB.size(),3);
	TU::IntegerColumn expected_col_b = {2,3,1};
	for (size_t i =0; i< colB.size(); ++i)
		EXPECT_EQ(colB[i], expected_col_b[i]);

	EXPECT_EQ(colC.size(),3);
	TU::IntegerColumn expected_col_c = {3,2,2};
	for (size_t i =0; i< colC.size(); ++i)
		EXPECT_EQ(colC[i], expected_col_c[i]);

	EXPECT_EQ(colD.size(),3);
	TU::IntegerColumn expected_col_d = {4,1,4};
	for (size_t i =0; i< colD.size(); ++i)
		EXPECT_EQ(colD[i], expected_col_d[i]);
}