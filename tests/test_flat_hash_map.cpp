#include "tubul.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
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

// Whether a map is looked up by std::string_view.
template <class Map>
concept FindsByView = requires(Map map, std::string_view view)
{ map.find(view); };

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

// A key built from text that counts how many times that happens, with a
// transparent hash and equality: lookups by text must not build one.
struct CountedKey
{
	explicit CountedKey(std::string_view text) : m_text(text)
	{
		++s_built;
	}
	std::string       m_text;
	static inline int s_built = 0;
};

struct CountedHash
{
	using is_transparent = void;
	size_t operator()(std::string_view text) const
	{
		return TU::DefaultHash<std::string_view>{}(text);
	}
	size_t operator()(const CountedKey &key) const
	{
		return (*this)(key.m_text);
	}
};

struct CountedEqual
{
	using is_transparent = void;
	static std::string_view text(std::string_view text)
	{
		return text;
	}
	static std::string_view text(const CountedKey &key)
	{
		return key.m_text;
	}
	template <class Lhs, class Rhs>
	bool operator()(const Lhs &lhs, const Rhs &rhs) const
	{
		return text(lhs) == text(rhs);
	}
};

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

// A map of strings is looked up by std::string_view and literals; a map of
// integers only by its key type.
TEST(FlatHashMapTest, TransparentLookupOfStrings)
{
	static_assert(requires(StringMap map, std::string_view view)
	{
		map.find(view);
		map.find("literal");
		map.contains(view);
		map.erase(view);
		map[view];
		map.try_emplace(view, 1);
	});
	static_assert(FindsByView<StringMap>);
	static_assert(!FindsByView<IntMap>);
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

// ---------------------------------------------------------------------------
// Insertions and element access
// ---------------------------------------------------------------------------

// insert() of a range or an initializer list inserts the pairs; the first of
// equal keys wins.
TEST(FlatHashMapTest, InsertRangeAndList)
{
	IntMap                                    map;
	const std::vector<std::pair<int, double>> pairs = {{1, 1.0}, {2, 2.0}, {1, 9.0}};
	map.insert(pairs.begin(), pairs.end());
	EXPECT_EQ(map.size(), 2U);
	EXPECT_EQ(map.find(1)->second, 1.0);
	map.insert({{3, 3.0}, {2, 9.0}});
	EXPECT_EQ(map.size(), 3U);
	EXPECT_EQ(map.find(2)->second, 2.0);
}

// operator[] inserts a value-initialized value for a new key, and returns
// the value of an existing one.
TEST(FlatHashMapTest, SubscriptOperator)
{
	IntMap map;
	EXPECT_EQ(map[1], 0.0);
	EXPECT_EQ(map.size(), 1U);
	map[1] = 5.0;
	map[2] += 1.5;
	EXPECT_EQ(map[1], 5.0);
	EXPECT_EQ(map[2], 1.5);
	EXPECT_EQ(map.size(), 2U);
}

// at() returns the value of a key, and throws std::out_of_range if the key
// isn't there.
TEST(FlatHashMapTest, At)
{
	IntMap map;
	map.insert({1, 1.5});
	map.at(1)              = 2.5;
	const IntMap &constMap = map;
	EXPECT_EQ(constMap.at(1), 2.5);
	EXPECT_THROW(map.at(2), std::out_of_range);
	EXPECT_THROW(constMap.at(2), std::out_of_range);
	EXPECT_EQ(map.size(), 1U);
}

// insert_or_assign() inserts a new key, or assigns the value of an existing
// one; the bool says which.
TEST(FlatHashMapTest, InsertOrAssign)
{
	IntMap map;
	EXPECT_TRUE(map.insert_or_assign(1, 1.5).second);
	const auto result = map.insert_or_assign(1, 2.5);
	EXPECT_FALSE(result.second);
	EXPECT_EQ(result.first->second, 2.5);
	EXPECT_EQ(map.size(), 1U);
}

// A key that is an lvalue is copied in by try_emplace(), insert_or_assign()
// and operator[], and stays as it was.
TEST(FlatHashMapTest, InsertionsOfAnLvalueKey)
{
	StringMap         map;
	const std::string key = "key";
	EXPECT_TRUE(map.try_emplace(key, 1).second);
	EXPECT_FALSE(map.insert_or_assign(key, 2).second);
	EXPECT_EQ(map[key], 2);
	EXPECT_EQ(key, "key");
	EXPECT_EQ(map.size(), 1U);
}

// try_emplace() builds the value from its arguments for a new key, and
// leaves them untouched for an existing one.
TEST(FlatHashMapTest, TryEmplace)
{
	TU::FlatHashMap<int, std::unique_ptr<int>> map;
	EXPECT_TRUE(map.try_emplace(1, std::make_unique<int>(10)).second);
	auto value = std::make_unique<int>(20);
	EXPECT_FALSE(map.try_emplace(1, std::move(value)).second);
	ASSERT_NE(value, nullptr);
	EXPECT_EQ(*map.find(1)->second, 10);
	TU::FlatHashMap<int, std::vector<int>> vectors;
	vectors.try_emplace(1, 3, 7);
	EXPECT_EQ(vectors.find(1)->second, std::vector<int>({7, 7, 7}));
}

// emplace() takes a key and a value, a pair, or the arguments of a pair; for
// an existing key, a key and a value are left untouched.
TEST(FlatHashMapTest, Emplace)
{
	TU::FlatHashMap<int, std::unique_ptr<int>> map;
	EXPECT_TRUE(map.emplace(1, std::make_unique<int>(1)).second);
	EXPECT_TRUE(map.emplace(std::pair{2, std::make_unique<int>(2)}).second);
	EXPECT_TRUE(map.emplace(std::piecewise_construct, std::forward_as_tuple(3), std::forward_as_tuple(new int(3)))
					.second);
	auto value = std::make_unique<int>(9);
	EXPECT_FALSE(map.emplace(1, std::move(value)).second);
	ASSERT_NE(value, nullptr);
	for (int key = 1; key <= 3; ++key)
		EXPECT_EQ(*map.find(key)->second, key);
}

// A map of strings is used with std::string_view and literals: finding,
// counting, erasing and assigning by text.
TEST(FlatHashMapTest, StringKeysByView)
{
	StringMap                           counts;
	const std::vector<std::string_view> words = {"a", "b", "a", "c", "a"};
	for (std::string_view word : words)
		++counts[word];
	EXPECT_EQ(counts.at("a"), 3);
	EXPECT_EQ(counts.find(std::string_view("b"))->second, 1);
	EXPECT_TRUE(counts.contains("c"));
	EXPECT_EQ(counts.count("d"), 0U);
	EXPECT_EQ(counts.erase(std::string_view("c")), 1U);
	EXPECT_FALSE(counts.insert_or_assign(std::string_view("a"), 7).second);
	EXPECT_EQ(counts["a"], 7);
	EXPECT_EQ(counts.size(), 2U);
}

// A transparent lookup builds no key; only inserting a new key builds one.
TEST(FlatHashMapTest, TransparentLookupBuildsNoKey)
{
	TU::FlatHashMap<CountedKey, int, CountedHash, CountedEqual> map;
	map.try_emplace(std::string_view("one"), 1);
	const int built = CountedKey::s_built;
	EXPECT_TRUE(map.contains(std::string_view("one")));
	EXPECT_EQ(map.find(std::string_view("two")), map.end());
	EXPECT_FALSE(map.try_emplace(std::string_view("one"), 5).second);
	EXPECT_EQ(map[std::string_view("one")], 1);
	EXPECT_EQ(map.erase(std::string_view("two")), 0U);
	EXPECT_EQ(CountedKey::s_built, built);
	map[std::string_view("two")] = 2;
	EXPECT_EQ(CountedKey::s_built, built + 1);
}

// Corner case: a value that refers to an element of the map is copied before
// an insertion that grows the table moves that element.
TEST(FlatHashMapTest, InsertingAValueOfTheMapWhileGrowing)
{
	TU::FlatHashMap<int, std::string> map;
	map.try_emplace(0, "a value longer than the small string buffer");
	int key = 1;
	while (map.size() < map.capacity() / 8 * 7)
		map.try_emplace(key++, "x");
	const size_t capacity = map.capacity();
	map.try_emplace(key, map.find(0)->second);
	EXPECT_GT(map.capacity(), capacity);
	EXPECT_EQ(map.find(key)->second, map.find(0)->second);
}
