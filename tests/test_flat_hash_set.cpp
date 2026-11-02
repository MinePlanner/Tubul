#include "tubul.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
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

// Whether a set is looked up by std::string_view.
template <class Set>
concept FindsByView = requires(Set set, std::string_view view)
{ set.find(view); };

// The keys of a set, sorted.
template <class Set>
std::vector<typename Set::key_type> sortedKeys(const Set &set)
{
	std::vector<typename Set::key_type> keys(set.begin(), set.end());
	std::sort(keys.begin(), keys.end());
	return keys;
}

// A hash that sends every key to the same place: lookups must still work.
struct ConstantHash
{
	size_t operator()(int) const
	{
		return 42;
	}
};

// A hash that isn't well mixed (no is_avalanching): the set mixes it.
struct IdentityHash
{
	size_t operator()(int key) const
	{
		return static_cast<size_t>(key);
	}
};

} // namespace

// ---------------------------------------------------------------------------
// Interface
// ---------------------------------------------------------------------------

// A set of strings is looked up by std::string_view and literals; a set of
// integers only by its key type.
TEST(FlatHashSetTest, TransparentLookupOfStrings)
{
	static_assert(requires(StringSet set, std::string_view view)
	{
		set.find(view);
		set.find("literal");
		set.contains(view);
		set.count(view);
		set.erase(view);
	});
	static_assert(FindsByView<StringSet>);
	static_assert(!FindsByView<IntSet>);
}

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

// emplace() builds the key from its arguments.
TEST(FlatHashSetTest, Emplace)
{
	StringSet set;
	EXPECT_TRUE(set.emplace(3, 'x').second);
	EXPECT_FALSE(set.emplace("xxx").second);
	EXPECT_TRUE(set.contains("xxx"));
}

// equal_range() is the range of a key: one key, or an empty range at end().
TEST(FlatHashSetTest, EqualRange)
{
	IntSet set;
	set.insert(1);
	const auto found = set.equal_range(1);
	EXPECT_EQ(found.first, set.find(1));
	EXPECT_EQ(std::next(found.first), found.second);
	const auto missing = set.equal_range(2);
	EXPECT_EQ(missing.first, set.end());
	EXPECT_EQ(missing.second, set.end());
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

// A hash that isn't well mixed, or one that gives every key the same hash,
// slows the set down but doesn't break it.
TEST(FlatHashSetTest, BadHashes)
{
	TU::FlatHashSet<int, IdentityHash> identity;
	TU::FlatHashSet<int, ConstantHash> constant;
	for (int key = 0; key < 200; ++key)
	{
		identity.insert(key * 1024);
		constant.insert(key);
	}
	for (int key = 0; key < 200; ++key)
	{
		EXPECT_TRUE(identity.contains(key * 1024));
		EXPECT_TRUE(constant.contains(key));
	}
	EXPECT_FALSE(identity.contains(1));
	EXPECT_FALSE(constant.contains(200));
}

// Corner cases: the extreme keys and zero.
TEST(FlatHashSetTest, ExtremeKeys)
{
	TU::FlatHashSet<int64_t> set;
	const int64_t            keys[] = {std::numeric_limits<int64_t>::min(), -1, 0, std::numeric_limits<int64_t>::max()};
	for (int64_t key : keys)
		set.insert(key);
	EXPECT_EQ(set.size(), 4U);
	for (int64_t key : keys)
		EXPECT_TRUE(set.contains(key));
	EXPECT_FALSE(set.contains(1));
}

// ---------------------------------------------------------------------------
// Erasure and size policy
// ---------------------------------------------------------------------------

// Erasing a key, an iterator (while iterating) and a range.
TEST(FlatHashSetTest, Erase)
{
	IntSet set;
	for (int key = 0; key < 1000; ++key)
		set.insert(key);
	EXPECT_EQ(set.erase(5), 1U);
	EXPECT_EQ(set.erase(5), 0U);
	for (auto it = set.begin(); it != set.end();)
	{
		if (*it % 2 == 1)
			it = set.erase(it);
		else
			++it;
	}
	EXPECT_EQ(set.size(), 500U);
	EXPECT_TRUE(set.contains(4));
	EXPECT_FALSE(set.contains(3));
	EXPECT_EQ(set.erase(set.begin(), set.end()), set.end());
	EXPECT_TRUE(set.empty());
}

// clear() keeps the memory; reserve(n) keeps keys in place until size() is
// n; erasing keeps the other keys in place; max_load_factor(float) changes
// nothing.
TEST(FlatHashSetTest, ClearReserveAndStableKeys)
{
	IntSet set;
	set.reserve(100);
	const size_t capacity = set.capacity();
	set.insert(0);
	const int *zero = &*set.find(0);
	for (int key = 1; key < 100; ++key)
		set.insert(key);
	set.erase(50);
	EXPECT_EQ(set.capacity(), capacity);
	EXPECT_EQ(&*set.find(0), zero);
	set.clear();
	EXPECT_TRUE(set.empty());
	EXPECT_EQ(set.capacity(), capacity);
	set.rehash(0);
	EXPECT_EQ(set.capacity(), 0U);
	EXPECT_EQ(set.load_factor(), 0.0F);
	EXPECT_EQ(set.max_load_factor(), 0.875F);
	set.max_load_factor(0.5F);
	EXPECT_EQ(set.max_load_factor(), 0.875F);
}

// Erasing invalidates only the erased key, and inserting a key that is there
// invalidates nothing.
TEST(FlatHashSetTest, ReinsertingKeepsKeysInPlace)
{
	IntSet set;
	for (int key = 0; key < 7; ++key)
		set.insert(key);
	std::vector<const int *> keys;
	for (int key = 0; key < 7; ++key)
		keys.push_back(&*set.find(key));
	set.erase(3);
	for (int key = 0; key < 7; ++key)
	{
		if (key != 3)
		{
			EXPECT_FALSE(set.insert(key).second);
			EXPECT_EQ(&*set.find(key), keys[key]);
		}
	}
}

// rehash(n) gives at least n slots, rehash(0) shrinks the table to fit, and
// the keys are kept.
TEST(FlatHashSetTest, Rehash)
{
	IntSet set;
	for (int key = 0; key < 100; ++key)
		set.insert(key);
	set.rehash(5000);
	EXPECT_GE(set.capacity(), 5000U);
	set.rehash(0);
	EXPECT_LT(set.capacity(), 5000U);
	EXPECT_GE(set.capacity() / 8 * 7, 100U);
	for (int key = 0; key < 100; ++key)
		EXPECT_TRUE(set.contains(key));
}

// bucket_count() is capacity(); max_size() bounds the size, and reserve()
// and rehash() above it throw std::length_error, leaving the set as it was.
TEST(FlatHashSetTest, SizeLimits)
{
	IntSet set;
	set.insert(1);
	EXPECT_EQ(set.bucket_count(), set.capacity());
	EXPECT_GT(set.max_size(), size_t{1} << 40);
	EXPECT_THROW(set.reserve(std::numeric_limits<size_t>::max()), std::length_error);
	EXPECT_THROW(set.rehash(set.max_size() + 1), std::length_error);
	EXPECT_EQ(set.size(), 1U);
	EXPECT_TRUE(set.contains(1));
}

// Corner case: erasing and inserting again and again (which leaves deleted
// slots) keeps the set right and the table from growing without end.
TEST(FlatHashSetTest, ChurnOfErasuresAndInsertions)
{
	IntSet set;
	for (int key = 0; key < 1000; ++key)
		set.insert(key);
	const size_t capacity = set.capacity();
	for (int key = 1000; key < 100000; ++key)
	{
		EXPECT_EQ(set.erase(key - 1000), 1U);
		EXPECT_TRUE(set.insert(key).second);
	}
	EXPECT_EQ(set.size(), 1000U);
	EXPECT_LE(set.capacity(), capacity * 2);
	for (int key = 99000; key < 100000; ++key)
		EXPECT_TRUE(set.contains(key));
	EXPECT_FALSE(set.contains(98999));
}
