#include "tubul.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// Tests of TU::FlatHashMap (tubul_flat_hash_map.h).

namespace
{

using IntMap    = TU::FlatHashMap<int, double>;
using StringMap = TU::FlatHashMap<std::string, int>;

// The keys of a map, sorted.
template <class Map>
std::vector<typename Map::key_type> sortedKeys(const Map &map)
{
	std::vector<typename Map::key_type> keys;
	for (const auto &element : map)
		keys.push_back(element.first);
	std::sort(keys.begin(), keys.end());
	return keys;
}

} // namespace

// ---------------------------------------------------------------------------
// Interface
// ---------------------------------------------------------------------------

// The member types of std::unordered_map, with its value_type.
TEST(FlatHashMapTest, MemberTypes)
{
	static_assert(std::is_same_v<IntMap::key_type, int>);
	static_assert(std::is_same_v<IntMap::mapped_type, double>);
	static_assert(std::is_same_v<IntMap::value_type, std::pair<const int, double>>);
	static_assert(std::is_same_v<IntMap::size_type, size_t>);
	static_assert(std::is_same_v<IntMap::difference_type, std::ptrdiff_t>);
	static_assert(std::is_same_v<IntMap::hasher, TU::DefaultHash<int>>);
	static_assert(std::is_same_v<IntMap::key_equal, TU::DefaultEqual<int>>);
	static_assert(std::is_same_v<IntMap::reference, IntMap::value_type &>);
	static_assert(std::is_same_v<IntMap::const_reference, const IntMap::value_type &>);
	static_assert(std::is_same_v<IntMap::pointer, IntMap::value_type *>);
	static_assert(std::is_same_v<IntMap::const_pointer, const IntMap::value_type *>);
	static_assert(!std::is_same_v<IntMap::iterator, IntMap::const_iterator>);
}

// What the insertions and lookups return, as in std::unordered_map.
TEST(FlatHashMapTest, ReturnTypes)
{
	using Iterator      = IntMap::iterator;
	using ConstIterator = IntMap::const_iterator;
	using InsertResult  = std::pair<Iterator, bool>;
	static_assert(requires(IntMap map, const IntMap constMap, IntMap::value_type value, int key)
	{
		{ map.insert(value) } -> std::same_as<InsertResult>;
		{ map.emplace(key, 1.0) } -> std::same_as<InsertResult>;
		{ map.try_emplace(key, 1.0) } -> std::same_as<InsertResult>;
		{ map.insert_or_assign(key, 1.0) } -> std::same_as<InsertResult>;
		{ map[key] } -> std::same_as<double &>;
		{ map.at(key) } -> std::same_as<double &>;
		{ constMap.at(key) } -> std::same_as<const double &>;
		{ map.find(key) } -> std::same_as<Iterator>;
		{ constMap.find(key) } -> std::same_as<ConstIterator>;
		{ constMap.contains(key) } -> std::same_as<bool>;
		{ constMap.count(key) } -> std::same_as<size_t>;
		{ map.erase(key) } -> std::same_as<size_t>;
		{ map.erase(constMap.begin()) } -> std::same_as<Iterator>;
	});
}

// ---------------------------------------------------------------------------
// Insertion, lookup and iteration
// ---------------------------------------------------------------------------

// A new map is empty, has no memory, and finds nothing.
TEST(FlatHashMapTest, EmptyMap)
{
	const IntMap map;
	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.size(), 0U);
	EXPECT_EQ(map.capacity(), 0U);
	EXPECT_EQ(map.begin(), map.end());
	EXPECT_EQ(map.cbegin(), map.cend());
	EXPECT_EQ(map.find(1), map.end());
	EXPECT_FALSE(map.contains(1));
	EXPECT_EQ(map.count(1), 0U);
}

// insert() returns the new element and true, and find() finds it.
TEST(FlatHashMapTest, InsertAndFind)
{
	IntMap     map;
	const auto result = map.insert({1, 1.5});
	const auto it     = result.first;
	EXPECT_TRUE(result.second);
	EXPECT_EQ(it->first, 1);
	EXPECT_EQ(it->second, 1.5);
	EXPECT_FALSE(map.empty());
	EXPECT_EQ(map.size(), 1U);
	EXPECT_EQ(map.find(1), it);
	EXPECT_TRUE(map.contains(1));
	EXPECT_EQ(map.count(1), 1U);
	EXPECT_EQ(map.find(2), map.end());
	const IntMap &constMap = map;
	EXPECT_EQ(constMap.find(1)->second, 1.5);
}

// Inserting a key that is there returns its element and false, and leaves
// its value as it was.
TEST(FlatHashMapTest, InsertExistingKey)
{
	IntMap     map;
	const auto first  = map.insert({1, 1.5});
	const auto second = map.insert({1, 2.5});
	EXPECT_FALSE(second.second);
	EXPECT_EQ(second.first, first.first);
	EXPECT_EQ(second.first->second, 1.5);
	EXPECT_EQ(map.size(), 1U);
}

// Values are written through iterators.
TEST(FlatHashMapTest, WriteThroughIterator)
{
	IntMap map;
	map.insert({1, 1.5});
	map.find(1)->second = 3.0;
	EXPECT_EQ(map.find(1)->second, 3.0);
	(*map.begin()).second = 4.0;
	EXPECT_EQ(map.find(1)->second, 4.0);
}

// Iteration visits every element once.
TEST(FlatHashMapTest, IterationVisitsEveryElementOnce)
{
	IntMap           map;
	std::vector<int> expected;
	for (int key = 0; key < 100; ++key)
	{
		map.insert({key * 7, key * 0.5});
		expected.push_back(key * 7);
	}
	EXPECT_EQ(sortedKeys(map), expected);
	size_t count = 0;
	for (auto it = map.cbegin(); it != map.cend(); it++)
		++count;
	EXPECT_EQ(count, map.size());
}

// Values that can only be moved are moved in, and survive the growth of the
// table.
TEST(FlatHashMapTest, MoveOnlyValues)
{
	TU::FlatHashMap<int, std::unique_ptr<int>> map;
	for (int key = 0; key < 100; ++key)
		map.insert({key, std::make_unique<int>(key)});
	for (int key = 0; key < 100; ++key)
		EXPECT_EQ(*map.find(key)->second, key);
}

// ---------------------------------------------------------------------------
// Erasure and load factor
// ---------------------------------------------------------------------------

// erase(key) returns how many elements it erased, 1 or 0.
TEST(FlatHashMapTest, EraseKey)
{
	IntMap map;
	map.insert({1, 1.0});
	map.insert({2, 2.0});
	EXPECT_EQ(map.erase(1), 1U);
	EXPECT_EQ(map.erase(1), 0U);
	EXPECT_EQ(map.erase(3), 0U);
	EXPECT_FALSE(map.contains(1));
	EXPECT_TRUE(map.contains(2));
	EXPECT_EQ(map.size(), 1U);
}

// erase(it) returns the next element, so the map can be erased while it is
// iterated; erasing part of a range returns its end, which is kept.
TEST(FlatHashMapTest, EraseWhileIteratingAndPartialRange)
{
	IntMap map;
	for (int key = 0; key < 100; ++key)
		map.insert({key, 0.0});
	for (auto it = map.begin(); it != map.end();)
	{
		if (it->first % 2 == 1)
			it = map.erase(it);
		else
			++it;
	}
	EXPECT_EQ(map.size(), 50U);
	EXPECT_FALSE(map.contains(1));
	const auto last    = std::next(map.cbegin(), 20);
	const int  lastKey = last->first;
	EXPECT_EQ(map.erase(map.cbegin(), last)->first, lastKey);
	EXPECT_EQ(map.size(), 30U);
	EXPECT_TRUE(map.contains(lastKey));
}

// equal_range() is the element of the key, or an empty range.
TEST(FlatHashMapTest, EqualRange)
{
	IntMap map;
	map.insert({1, 1.0});
	const auto found = map.equal_range(1);
	EXPECT_EQ(found.first, map.find(1));
	EXPECT_EQ(std::next(found.first), found.second);
	const auto missing = map.equal_range(2);
	EXPECT_EQ(missing.first, map.end());
	EXPECT_EQ(missing.second, map.end());
}

// erase(first, last) erases a range; the whole map leaves it empty.
TEST(FlatHashMapTest, EraseRange)
{
	IntMap map;
	for (int key = 0; key < 100; ++key)
		map.insert({key, 0.0});
	EXPECT_EQ(map.erase(map.begin(), map.end()), map.end());
	EXPECT_TRUE(map.empty());
	map.insert({1, 1.0});
	EXPECT_EQ(map.erase(map.cbegin(), map.cbegin()), map.begin());
	EXPECT_EQ(map.size(), 1U);
}

// clear() erases everything but keeps the memory, and the map is usable.
TEST(FlatHashMapTest, Clear)
{
	IntMap map;
	for (int key = 0; key < 100; ++key)
		map.insert({key, 0.0});
	const size_t capacity = map.capacity();
	map.clear();
	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.begin(), map.end());
	EXPECT_EQ(map.capacity(), capacity);
	EXPECT_FALSE(map.contains(5));
	map.insert({5, 5.0});
	EXPECT_EQ(map.find(5)->second, 5.0);
}
