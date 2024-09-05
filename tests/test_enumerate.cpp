//
// Created by Carlos Acosta on 19-10-23.
//
#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>
#include <set>
#include <deque>


TEST(TUBULEnumerate, vector) {
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

TEST(TUBULZip, vector_set) {

    std::vector<int> test_vector  = { 1,2,3,4,5};
    std::set<char> test_set= { 'a', 'b', 'c', 'd', 'e'};
    auto index_vec = test_vector.begin();
    auto index_set = test_set.begin();
    //The range can be grabbed and reused
    auto zipped = TU::zip(test_vector, test_set);

    for (const auto &[num, letter]: zipped) {
        // std::cout <<" tuple [" << num << "," << letter << "]" << std::endl;
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, letter);
        ++index_vec;
        ++index_set;
    }

    //We have to reset the expected values
    index_vec = test_vector.begin();
    index_set = test_set.begin();
    for (const auto &[num, letter]: zipped) {
        // std::cout <<" tuple [" << num << "," << letter << "]" << std::endl;
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, letter);
        ++index_vec;
        ++index_set;
    }
}

TEST(TUBULZip, vector_set2)
{

    std::vector<int> test_vector  = { 1,2,3,4,5,6};
    std::set<std::string> test_set= { "first", "second", "third", "fourth"};
    auto index_vec = test_vector.begin();
    auto index_set = test_set.begin();

    for (const auto &[num, letter]: TU::zip(test_vector, test_set)) {
        // std::cout <<" tuple [" << num << "," << letter << "]" << std::endl;
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, letter);
        ++index_vec;
        ++index_set;
    }
}

TEST(TUBULZip, vector_set3)
{

    std::vector<int> test_vector  = { 1,2,3,4,5,6};
    std::set<std::string> test_set= { "first", "second", "third", "fourth"};
    std::deque<char> test_deque= { 'a', 'b', 'c', 'd', 'e'};
    auto index_vec = test_vector.begin();
    auto index_set = test_set.begin();
    auto index_deque = test_deque.begin();

    for (const auto &[num, word, letter]: TU::zip(test_vector, test_set, test_deque)) {
         std::cout <<" tuple [" << num << "," << word << "," << letter << "]" << std::endl;
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, word);
        EXPECT_EQ(*index_deque, letter);
        ++index_vec;
        ++index_set;
        ++index_deque;
    }
}

TEST(TUBULZip, std_algorithm)
{

    std::vector<int> test_vector  = { 1,2,3,4,5,6};
    std::set<std::string> test_set= { "first", "second", "third", "fourth"};
    std::deque<char> test_deque= { 'a', 'b', 'c', 'd', 'e'};

    //Do note we have to change the order of the elements in the set because
    //it's a set, so they will look re-ordered when iterating from begin() to end().
    std::vector<std::tuple<int, std::string, char>> expected = {
            { 1, "first", 'a'},
            { 2, "fourth", 'b'},
            { 3, "second", 'c'},
            { 4, "third", 'd'}
        };

    auto zipped = TU::zip(test_vector, test_set, test_deque);
    //Use std::equal between the zip-view and the actual container with the expected results
    auto equal_result = std::equal(expected.begin(), expected.end(), zipped.begin());
    EXPECT_TRUE(equal_result);
}
