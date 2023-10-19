//
// Created by Carlos Acosta on 19-10-23.
//
#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>
#include <set>


TEST(TUBULEnumerate, vector)
{
   std::vector<char> test_vector  = { 'a', 'b', 'c', 'd', 'e'};
   std::vector<std::tuple<size_t, char>> expected = {  {0,'a'}, {1,'b'}, {2,'c'},{3,'d'},{4,'e'} };
   auto index = 0;
   for ( const auto& [i, item]: TU::enumerate(test_vector)){
       const auto& [expected_i, expected_item] = expected[index];
       EXPECT_EQ(expected_i,i);
       EXPECT_EQ(expected_item, item);

       ++index;
   }
    EXPECT_EQ(index,5);
}

TEST(TUBULEnumerate, set)
{
    std::set<int> test_vector  = { 10,5,20,15,25};
    std::vector<std::tuple<size_t, int>> expected = {  {0,5}, {1,10}, {2,15},{3,20},{4,25} };
    auto index = 0;
    for ( const auto& [i, item]: TU::enumerate(test_vector)){
        const auto& [expected_i, expected_item] = expected[index];
        EXPECT_EQ(expected_i,i);
        EXPECT_EQ(expected_item, item);

        ++index;
    }
    EXPECT_EQ(index,5);
}

