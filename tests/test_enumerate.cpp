//
// Created by Carlos Acosta on 19-10-23.
//
#include <gtest/gtest.h>
#include "tubul.h"
#include <vector>
#include <set>
#include <deque>
#include <ranges>
#include <format>


TEST(TUBULEnumerate, vector) {
   std::vector<char> test_vector  = { 'a', 'b', 'c', 'd', 'e', 'f', 'g'};
   std::vector<std::tuple<size_t, char>> expected = {  {0,'a'}, {1,'b'}, {2,'c'},{3,'d'},{4,'e'}, {5,'f'}, {6,'g'} };
   auto index = 0;
   for ( const auto& [i, item]: TU::enumerate(test_vector)){
       const auto& [expected_i, expected_item] = expected[index];
       EXPECT_EQ(expected_i,i);
       EXPECT_EQ(expected_item, item);

       ++index;
   }
    EXPECT_EQ(index,7);

	index = 0;
	//An enumerate range stored as an lvalue, must be a view
	auto enumRange = TU::enumerate(test_vector);
	static_assert(std::ranges::view<decltype(enumRange)>, "enumerate is not a range");
	//Because the original range has a size, then the enumerated range can have size
	ASSERT_EQ(enumRange.size(), expected.size());
	//An enumerate range as an rvalue also has to be compliant with view so we can compose it.
	static_assert(std::ranges::view<decltype(TU::enumerate(test_vector))>, "enumerate is not a range");
	//And in fact, we should be able to compose it.
	auto composedEnum =   TU::enumerate(test_vector) |
	 std::views::transform([](const auto &i)
	{
		const auto &[idx, item] = i;
		return std::to_string(idx+10) + "->" + item;
	});

	std::ranges::for_each(composedEnum, [](const auto &i)
						  { std::cout << i << std::endl; });

	//Checking if we can create an enumerate view over a composed view
	auto test_view = test_vector | std::views::transform([](char i){ return ++i; });
	for ( const auto&[ idx, item]: TU::enumerate(test_view))
	{
		std::cout <<  std::format("{} -> {}\n", idx, item);
	}
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
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, letter);
        ++index_vec;
        ++index_set;
    }

    //We have to reset the expected values
    index_vec = test_vector.begin();
    index_set = test_set.begin();
    for (const auto &[num, letter]: zipped) {
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, letter);
        ++index_vec;
        ++index_set;
    }

	//We can use TU::zip with a non-const view (the main concern is a mutable lambda
	//such as enumerate) and zip still can deduce normally the correct types.
	auto enumTest = TU::enumerate(test_set) | std::views::transform([](const auto &item){auto [idx, val] = item; return std::make_tuple(idx+10, val);} );
	for (const auto &[num, letter]: enumTest)
		std::cout << std::format("normal {} -> {}\n", num, letter);
	 for (const auto &[tup, letter]: TU::zip(enumTest, test_set))
	 {
	 	const auto&[ idx, tupletter] = tup;
		 std::cout << std::format("zip [{},{}] -> {}\n", idx, tupletter, letter);
	 }
}

TEST(TUBULZip, vector_set2)
{

    std::vector<int> test_vector  = { 1,2,3,4,5,6};
    std::set<std::string> test_set= { "first", "second", "third", "fourth"};
    auto index_vec = test_vector.begin();
    auto index_set = test_set.begin();

    for (const auto &[num, letter]: TU::zip(test_vector, test_set)) {
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, letter);
        ++index_vec;
        ++index_set;
    }

	index_vec = test_vector.begin() ;
	std::advance(index_vec, 3);
    index_set = test_set.begin();
	std::advance(index_set, 3);
    for (const auto &[num, letter]: TU::zip(test_vector, test_set) | std::views::filter([](const auto &i){ return std::get<0>(i) > 3; })) {
        EXPECT_EQ(*index_vec, num);
        EXPECT_EQ(*index_set, letter);
        ++index_vec;
        ++index_set;
    }

	//Now we will create a new view, and pass it to zip to test that we can receive
	//these types of views (which can cause a lot of issues due const-ness
	std::string buffer; //We need a value to reference to, because zip references back the items.
	auto vecView = test_vector | std::views::transform([](const int& i){ return i + 100;} );
	auto setView = test_set | std::views::transform([&buffer](const std::string& i)->const std::string& { buffer = i; buffer[0] = buffer.back(); return buffer;} );
	for (const auto &[num, txt]: TU::zip(vecView, setView))
	{
		std::cout << std::format("{} -> {}\n", num, txt);
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
