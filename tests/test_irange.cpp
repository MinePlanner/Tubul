//
// Created by Carlos Acosta on 20-01-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>

TEST(TUBULIrange, testBasicFunctionality)
{
	std::vector<size_t> expected = {0, 1, 2, 3, 4};
	size_t iter_count = 0;

	//Checking simple range from 0 to X (in this case, 5)
	for (auto i : TU::irange(5))
	{
		EXPECT_EQ(i, expected[iter_count]);
		iter_count++;
	}
	EXPECT_EQ(expected.size(), iter_count);

	//This should be the same as iterating from 0 to size of a vector.
	iter_count = 0;
	for (auto i : TU::irange(expected.size()))
	{
		EXPECT_EQ(i, expected[iter_count]);
		iter_count++;
	}
	EXPECT_EQ(expected.size(), iter_count);
	//also check passing the starting index.
	iter_count = 0;
	for (auto i : TU::irange(0, expected.size()))
	{
		EXPECT_EQ(i, expected[iter_count]);
		iter_count++;
	}
	EXPECT_EQ(expected.size(), iter_count);

	iter_count = 3;
	for (auto i : TU::irange(3, expected.size()))
	{
		EXPECT_EQ(i, expected[iter_count]);
		iter_count++;
	}
	EXPECT_EQ(expected.size(), iter_count);

	//Iterating from 5 until the end.
	expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	iter_count = 5;
	for (auto i : TU::irange(iter_count,expected.size()))
	{
		EXPECT_EQ(i, expected[iter_count]);
		iter_count++;
	}
	EXPECT_EQ(expected.size(), iter_count);

}

TEST(TUBULIrange, testSkipNumbers)
{
	//Testing "even numbers smaller than 11"
	std::vector<size_t> expected = {0, 2, 4, 6, 8, 10};
	size_t iter_count = 0;
	for (auto i: TU::irange(0,11,2))
	{
		EXPECT_EQ(i, expected[iter_count]);
		iter_count++;
	}
	EXPECT_EQ(expected.size(), iter_count);

	//Numbers smaller than 11, skipping by 3.
	//Expect 0,3,6,9
	iter_count = 0;
	for (auto i: TU::irange(0,11,3))
	{
		std::cout << "it(" << iter_count << ") = " << i << std::endl;
		EXPECT_EQ(i, iter_count*3);
		iter_count++;
	}
	EXPECT_EQ(4, iter_count);
}

