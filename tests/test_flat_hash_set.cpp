#include "tubul.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// Tests of TU::FlatHashSet (tubul_flat_hash_set.h).

namespace
{

using IntSet    = TU::FlatHashSet<int>;
using StringSet = TU::FlatHashSet<std::string>;

// The keys of a set, sorted.
template <class Set>
std::vector<typename Set::key_type> sortedKeys(const Set &set)
{
	std::vector<typename Set::key_type> keys(set.begin(), set.end());
	std::sort(keys.begin(), keys.end());
	return keys;
}

} // namespace

// ---------------------------------------------------------------------------
// Interface
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Insertion, lookup and iteration
// ---------------------------------------------------------------------------

// A new set is empty, has no memory, and finds nothing.
TEST(FlatHashSetTest, EmptySet)
{
	const IntSet set;
	EXPECT_TRUE(set.empty());
	EXPECT_EQ(set.size(), 0U);
	EXPECT_EQ(set.capacity(), 0U);
	EXPECT_EQ(set.begin(), set.end());
	EXPECT_FALSE(set.contains(1));
	EXPECT_EQ(set.count(1), 0U);
}

// insert() returns the key in the set and whether it was inserted.
TEST(FlatHashSetTest, InsertAndFind)
{
	IntSet     set;
	const auto first = set.insert(1);
	EXPECT_TRUE(first.second);
	EXPECT_EQ(*first.first, 1);
	const auto second = set.insert(1);
	EXPECT_FALSE(second.second);
	EXPECT_EQ(second.first, first.first);
	EXPECT_EQ(set.find(1), first.first);
	EXPECT_EQ(set.find(2), set.end());
	EXPECT_EQ(set.count(1), 1U);
	EXPECT_EQ(set.size(), 1U);
}

// cbegin() and cend() are begin() and end(), it++ returns the position
// before it moves, and -> reaches the key.
TEST(FlatHashSetTest, IteratorOperations)
{
	StringSet set;
	set.insert("a");
	set.insert("bb");
	set.insert("ccc");
	EXPECT_EQ(set.cbegin(), set.begin());
	EXPECT_EQ(set.cend(), set.end());
	size_t length = 0;
	for (auto it = set.cbegin(); it != set.cend();)
	{
		const auto old = it++;
		EXPECT_NE(old, it);
		length += old->size();
	}
	EXPECT_EQ(length, 6U);
}

// The set grows as needed, and iteration visits every key once.
TEST(FlatHashSetTest, GrowsAndIteratesEveryKeyOnce)
{
	IntSet           set;
	std::vector<int> expected;
	for (int key = 0; key < 10000; ++key)
	{
		EXPECT_TRUE(set.insert(key * 3).second);
		expected.push_back(key * 3);
	}
	EXPECT_EQ(sortedKeys(set), expected);
	EXPECT_FALSE(set.contains(1));
	EXPECT_LE(set.size(), set.capacity() / 8 * 7);
}
