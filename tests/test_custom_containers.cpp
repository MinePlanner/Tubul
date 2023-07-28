//
// Created by Carlos Acosta on 28-07-23.
//

#include "tubul.h"
#include <gtest/gtest.h>

TEST(TUBULContainers, testFlatMapBasic) {

    auto printAssociativeContainer = [](auto container) -> void {
        for (auto &[key, val]: container) {
            std::cout << "key:" << key << "  -> value:" << val << std::endl;
        }

    };

    //Basic check for empty map
    TU::FlatMap<int, int> test;
    EXPECT_TRUE( test.empty());
    EXPECT_EQ( test.size(), 0 );
    EXPECT_EQ( test.capacity(), 0 );

    test.reserve(8);
    EXPECT_TRUE( test.empty());
    EXPECT_EQ( test.size(), 0 );
    EXPECT_EQ( test.capacity(), 8 );

    //After inserting one element
    test.insert({1, 10});
    EXPECT_EQ( test.size(), 1);
    EXPECT_FALSE( test.empty());

    //Inserting another...
    test.insert({8, 20});
    EXPECT_EQ( test.size(), 2);

    //Inserting another...
    test.insert({12, 30});
    EXPECT_EQ( test.size(), 3);
    EXPECT_FALSE( test.empty());

    //check what we inserted
    EXPECT_TRUE( test.contains(1));
    EXPECT_TRUE( test.contains(8));
    EXPECT_TRUE( test.contains(12));
    EXPECT_EQ( test[1], 10) ;
    EXPECT_EQ( test[8], 20) ;
    EXPECT_EQ( test[12], 30) ;
    EXPECT_EQ( test.capacity(), 8 );

    //Let's try to insert a repeated element. It should say so
    //and the iterator points to current value.
    auto [it, newInsertion] = test.insert({8, 40});
    EXPECT_FALSE(newInsertion);
    EXPECT_EQ(*it, std::make_pair(8,20));
    EXPECT_EQ( test.size(), 3);

    //But we can overwrite what is stored in that key
    test[8] = 40;
    EXPECT_EQ(test[8], 40);
    EXPECT_EQ( test.size(), 3);

    //Also when the items stored should be ordered.
    std::vector<int> expected = {1,8,12};
    size_t expectedIt = 0;
    for ( auto& [key,val]: test ){
        EXPECT_EQ(key, expected[expectedIt]);
        ++expectedIt;
    }
    //Now let's insert a truly new element, but should be after 1
    auto [it2, newInsertion2] = test.insert({2, 77});
    EXPECT_TRUE(newInsertion2);
    EXPECT_EQ(*it2, std::make_pair(2,77));
    EXPECT_EQ( test.size(), 4);
    EXPECT_EQ( test.capacity(), 8);
    EXPECT_EQ( test[2], 77);

    std::vector<int> expected2 = {1,2,8,12};
    expectedIt = 0;
    for ( auto& [key,val]: test ){
        EXPECT_EQ(key, expected2[expectedIt]);
        ++expectedIt;
    }
}



TEST(TUBULContainers, testFlatMapConstructorList) {

    //It works creating maps from an initializer list
    TU::FlatMap<int,int> test = { {1,10}, {2,20}, {3,30} };
    EXPECT_EQ( test.size(), 3);

    size_t expectedIt = 1;
    for ( auto& [key,val]: test ){
        EXPECT_EQ(key, expectedIt);
        EXPECT_EQ(val, expectedIt*10);
        ++expectedIt;
    }

    //Even if the elements in the list are not ordered
    TU::FlatMap<int,int> test2 = {  {3,30}, {2,20},{1,10} };
    EXPECT_EQ( test.size(), 3);
    expectedIt = 1;
    for ( auto& [key,val]: test2 ){
        EXPECT_EQ(key, expectedIt);
        EXPECT_EQ(val, expectedIt*10);
        ++expectedIt;
    }
}

TEST(TUBULContainers, testFlatSetBasic) {

    auto printContainer = [](auto container) -> void {
        for (auto &val: container) {
            std::cout << " value:" << val << std::endl;
        }

    };

    //Basic check for empty set
    TU::FlatSet<int> test;
    EXPECT_TRUE( test.empty());
    EXPECT_EQ( test.size(), 0 );
    EXPECT_EQ( test.capacity(), 0 );

    test.reserve(8);
    EXPECT_TRUE( test.empty());
    EXPECT_EQ( test.size(), 0 );
    EXPECT_EQ( test.capacity(), 8 );

    //After inserting one element
    test.insert(1);
    EXPECT_EQ( test.size(), 1);
    EXPECT_FALSE( test.empty());

    //Inserting another...
    test.insert(8);
    EXPECT_EQ( test.size(), 2);

    //Inserting another...
    test.insert(12);
    EXPECT_EQ( test.size(), 3);
    EXPECT_FALSE( test.empty());

    //check what we inserted
    EXPECT_TRUE( test.contains(1));
    EXPECT_TRUE( test.contains(8));
    EXPECT_TRUE( test.contains(12));
    EXPECT_EQ( test.capacity(), 8 );

    //Let's try to insert a repeated element. It should say so
    //and the iterator points to current value.
    auto [it, newInsertion] = test.insert(8);
    EXPECT_FALSE(newInsertion);
    EXPECT_EQ(*it, 8);
    EXPECT_EQ( test.size(), 3);

    //Also when the items stored should be ordered.
    std::vector<int> expected = {1,8,12};
    size_t expectedIt = 0;
    for ( auto& key: test ){
        EXPECT_EQ(key, expected[expectedIt]);
        ++expectedIt;
    }
    //Now let's insert a truly new element, but should be after 1
    auto [it2, newInsertion2] = test.insert(2 );
    EXPECT_TRUE(newInsertion2);
    EXPECT_EQ(*it2, 2);
    EXPECT_EQ( test.size(), 4);
    EXPECT_EQ( test.capacity(), 8);

    std::vector<int> expected2 = {1,2,8,12};
    expectedIt = 0;
    for ( auto& val: test ){
        EXPECT_EQ(val, expected2[expectedIt]);
        ++expectedIt;
    }
}
TEST(TUBULContainers, testFlatSetConstructorList) {

    //It works creating maps from an initializer list
    TU::FlatSet<int> test = { 1, 2, 3 };
    EXPECT_EQ( test.size(), 3);

    size_t expectedIt = 1;
    for ( auto& val: test ){
        EXPECT_EQ(val, expectedIt);
        ++expectedIt;
    }

    //Even if the elements in the list are not ordered
    TU::FlatSet<int> test2 = {  3,2,1 };
    EXPECT_EQ( test.size(), 3);
    expectedIt = 1;
    for ( auto& key: test2 ){
        EXPECT_EQ(key, expectedIt);
        ++expectedIt;
    }
    test2.clear();

}
TEST(TUBULContainers, testFlatSetFind) {
    std::vector<int> sample = { 1, 2, 3, 4, 5 };
    TU::FlatSet<int> test = { 2, 3, 1, 5, 4 };

    EXPECT_EQ(test.size(),5);
    for (auto n: sample )
        EXPECT_TRUE( test.contains(n));

    auto it = test.find(3);
    EXPECT_FALSE(it == test.end());
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(*(it-1), 2);
    EXPECT_EQ(*(it+1), 4);

    auto notfound = test.find(10);
    EXPECT_TRUE(notfound == test.end());

    auto first = test.begin();
    EXPECT_FALSE(first == test.end());
    EXPECT_EQ(*first, 1);
    EXPECT_EQ(*(first+1), 2);
    auto lookFor1 = test.find(1);
    EXPECT_EQ( first, lookFor1 );

    auto last = test.end() - 1;
    EXPECT_FALSE(last == test.end());
    EXPECT_EQ(*last, 5);
    EXPECT_EQ(*(last-1), 4);
    EXPECT_EQ(last+1, test.end());
    auto lookFor5 = test.find(5);
    EXPECT_EQ(lookFor5, last);


}
