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

// A hash with state, which copies of the set and hash_function() keep.
struct SeededHash
{
	uint64_t m_seed = 0;
	size_t   operator()(int key) const
	{
		return TU::DefaultHash<uint64_t>{}(m_seed ^ static_cast<uint64_t>(key));
	}
};

// A key that can only be moved, and counts the live instances.
struct MoveOnlyKey
{
	explicit MoveOnlyKey(int id) : m_id(id)
	{
		++s_live;
	}
	MoveOnlyKey(MoveOnlyKey &&other) noexcept : m_id(other.m_id)
	{
		++s_live;
	}
	MoveOnlyKey &operator=(MoveOnlyKey &&) noexcept = default;
	~MoveOnlyKey()
	{
		--s_live;
	}
	bool operator==(const MoveOnlyKey &other) const
	{
		return m_id == other.m_id;
	}
	friend auto hashFields(const MoveOnlyKey &key)
	{
		return std::tie(key.m_id);
	}
	int               m_id;
	static inline int s_live = 0;
};

} // namespace

// ---------------------------------------------------------------------------
// Interface
// ---------------------------------------------------------------------------

// The member types of std::unordered_set; both iterators are constant.
TEST(FlatHashSetTest, MemberTypes)
{
	static_assert(std::is_same_v<IntSet::key_type, int>);
	static_assert(std::is_same_v<IntSet::value_type, int>);
	static_assert(std::is_same_v<IntSet::size_type, size_t>);
	static_assert(std::is_same_v<IntSet::difference_type, std::ptrdiff_t>);
	static_assert(std::is_same_v<IntSet::hasher, TU::DefaultHash<int>>);
	static_assert(std::is_same_v<IntSet::key_equal, TU::DefaultEqual<int>>);
	static_assert(std::is_same_v<IntSet::reference, int &>);
	static_assert(std::is_same_v<IntSet::const_reference, const int &>);
	static_assert(std::is_same_v<IntSet::iterator, IntSet::const_iterator>);
	static_assert(std::is_same_v<std::iter_reference_t<IntSet::iterator>, const int &>);
}

// What the insertions and lookups return, as in std::unordered_set.
TEST(FlatHashSetTest, ReturnTypes)
{
	using Iterator     = IntSet::iterator;
	using InsertResult = std::pair<Iterator, bool>;
	static_assert(requires(IntSet set, const IntSet constSet, int key)
	{
		{ set.insert(key) } -> std::same_as<InsertResult>;
		{ set.emplace(key) } -> std::same_as<InsertResult>;
		{ constSet.find(key) } -> std::same_as<Iterator>;
		{ constSet.contains(key) } -> std::same_as<bool>;
		{ constSet.count(key) } -> std::same_as<size_t>;
		{ set.erase(key) } -> std::same_as<size_t>;
		{ set.erase(constSet.begin()) } -> std::same_as<Iterator>;
	});
}

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

// The deduction guides take the key type from the keys.
TEST(FlatHashSetTest, DeductionGuides)
{
	using FromList = decltype(TU::FlatHashSet{1, 2, 3});
	static_assert(std::is_same_v<FromList, IntSet>);
	using Keys      = std::string *;
	using FromRange = decltype(TU::FlatHashSet(Keys(), Keys()));
	static_assert(std::is_same_v<FromRange, StringSet>);
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

// A set of strings is searched with std::string_view and literals.
TEST(FlatHashSetTest, StringKeysByView)
{
	StringSet set = {"one", "two"};
	EXPECT_TRUE(set.contains(std::string_view("one")));
	EXPECT_EQ(*set.find("two"), "two");
	EXPECT_EQ(set.count("three"), 0U);
	EXPECT_EQ(set.equal_range("one").first, set.find("one"));
	EXPECT_EQ(set.erase(std::string_view("one")), 1U);
	EXPECT_EQ(set.size(), 1U);
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

// ---------------------------------------------------------------------------
// The whole set
// ---------------------------------------------------------------------------

// The constructors, the deduction guides and equality.
TEST(FlatHashSetTest, ConstructorsAndEquality)
{
	const std::vector<int> keys = {3, 1, 2, 3};
	const IntSet           fromRange(keys.begin(), keys.end());
	EXPECT_EQ(fromRange.size(), 3U);
	const IntSet fromList = {1, 2, 3};
	EXPECT_EQ(fromRange, fromList);
	const TU::FlatHashSet deduced(keys.begin(), keys.end());
	EXPECT_EQ(deduced, fromList);
	IntSet assigned = {9};
	assigned        = {1, 2, 3};
	EXPECT_EQ(assigned, fromList);
	EXPECT_NE(assigned, IntSet({1, 2}));
	EXPECT_GE(IntSet(100).capacity() / 8 * 7, 100U);
}

// Copies are independent; a moved-from set is empty, without memory; swap()
// keeps references valid.
TEST(FlatHashSetTest, CopyMoveAndSwap)
{
	IntSet original = {1, 2};
	IntSet copy     = original;
	copy.erase(1);
	EXPECT_TRUE(original.contains(1));
	IntSet moved = std::move(original);
	EXPECT_EQ(moved, IntSet({1, 2}));
	EXPECT_TRUE(original.empty());
	EXPECT_EQ(original.capacity(), 0U);
	const int *one = &*moved.find(1);
	swap(moved, copy);
	EXPECT_EQ(&*copy.find(1), one);
	EXPECT_EQ(moved, IntSet({2}));
}

// Copy and move assignment replace the keys of a set that has some, a set
// moved into itself stays usable, and the hash and equality of a set, with
// their state, go with its copies.
TEST(FlatHashSetTest, AssignmentAndObservers)
{
	const IntSet source = {1, 2, 3};
	IntSet       copied = {7, 8};
	copied              = source;
	EXPECT_EQ(copied, source);
	IntSet moved = {9};
	moved        = std::move(copied);
	EXPECT_EQ(moved, source);
	IntSet &self = moved;
	moved        = std::move(self);
	EXPECT_EQ(moved.size(), static_cast<size_t>(std::distance(moved.begin(), moved.end())));
	TU::FlatHashSet<int, SeededHash> seeded(0, SeededHash{12345});
	seeded.insert(1);
	const auto copy = seeded;
	EXPECT_EQ(copy.hash_function().m_seed, 12345U);
	EXPECT_TRUE(copy.contains(1));
	EXPECT_TRUE(copy.key_eq()(1, 1));
}

// erase_if() erases the keys for which the predicate is true.
TEST(FlatHashSetTest, EraseIf)
{
	IntSet set;
	for (int key = 0; key < 100; ++key)
		set.insert(key);
	EXPECT_EQ(erase_if(set, [](int key)
					   { return key < 30; }),
			  30U);
	EXPECT_EQ(set.size(), 70U);
	EXPECT_FALSE(set.contains(29));
}

// Corner cases: self-assignment and self-swap change nothing, and a copy of a
// set with erased keys has only the others.
TEST(FlatHashSetTest, SelfAssignmentAndCopiesAfterErasures)
{
	IntSet set;
	for (int key = 0; key < 100; ++key)
		set.insert(key);
	for (int key = 0; key < 100; key += 3)
		set.erase(key);
	const IntSet expected = set;
	IntSet      &self     = set;
	set                   = self;
	set.swap(self);
	EXPECT_EQ(set, expected);
	EXPECT_EQ(expected.size(), 66U);
	EXPECT_FALSE(expected.contains(0));
	EXPECT_TRUE(expected.contains(1));
}

// Corner case: keys that can only be moved survive the growth of the table,
// and every key is destroyed exactly once.
TEST(FlatHashSetTest, MoveOnlyKeysAreDestroyedOnce)
{
	{
		TU::FlatHashSet<MoveOnlyKey> set;
		for (int id = 0; id < 1000; ++id)
			set.emplace(id);
		EXPECT_EQ(MoveOnlyKey::s_live, 1000);
		for (int id = 0; id < 1000; ++id)
			EXPECT_TRUE(set.contains(MoveOnlyKey(id)));
		EXPECT_EQ(set.erase(MoveOnlyKey(7)), 1U);
		EXPECT_EQ(MoveOnlyKey::s_live, 999);
		auto moved = std::move(set);
		EXPECT_EQ(MoveOnlyKey::s_live, 999);
	}
	EXPECT_EQ(MoveOnlyKey::s_live, 0);
}
