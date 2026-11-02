#include "tubul.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Tests of TU::DefaultHash and TU::DefaultEqual (tubul_flat_hash.h).

namespace
{

template <class Hash, class Key>
concept HashesTo = requires(const Hash &hash, const Key &key)
{
	{ hash(key) } -> std::same_as<size_t>;
};

template <class Equal, class Key>
concept Compares = requires(const Equal &equal, const Key &key)
{
	{ equal(key, key) } -> std::same_as<bool>;
};

template <class T>
concept IsTransparent = requires
{ typename T::is_transparent; };

template <class T>
concept IsAvalanching = requires
{ typename T::is_avalanching; };

enum class Level : int8_t
{
	kLow  = -128,
	kZero = 0,
	kHigh = 127
};
enum class Wide : uint64_t
{
	kSmall = 1,
	kHuge  = 0xFFFFFFFFFFFFFFFFULL
};

// A key without padding, hashed by its bytes, compared by them too.
struct EdgeTime
{
	uint64_t m_edge;
	int32_t  m_time;
	uint32_t m_slot;
};
static_assert(std::has_unique_object_representations_v<EdgeTime>);

// A key with padding and a std::hash (below) that ignores the padding.
struct Padded
{
	uint64_t m_id;
	uint8_t  m_kind;
	bool     operator==(const Padded &other) const
	{
		return m_id == other.m_id && m_kind == other.m_kind;
	}
};
static_assert(!std::has_unique_object_representations_v<Padded>);

// The example of tubul_flat_hash.h: padding, an operator== and hashFields().
struct CategoryKey
{
	uint64_t    m_category;
	int32_t     m_time;
	uint16_t    m_realization;
	bool        operator==(const CategoryKey &) const = default;
	friend auto hashFields(const CategoryKey &key)
	{
		return std::tie(key.m_category, key.m_time, key.m_realization);
	}
};

// A key without operator== whose hashFields() leaves m_cache out: both the
// hash and the equality ignore it.
struct IdAndCache
{
	double      m_weight;
	int32_t     m_id;
	int32_t     m_cache;
	friend auto hashFields(const IdAndCache &key)
	{
		return std::tie(key.m_weight, key.m_id);
	}
};

// A key whose operator== only compares m_id, as its hashFields() does.
struct Named
{
	int32_t     m_id;
	std::string m_name;
	bool        operator==(const Named &other) const
	{
		return m_id == other.m_id;
	}
	friend auto hashFields(const Named &key)
	{
		return std::tie(key.m_id);
	}
};

template <class Key>
size_t hashOf(const Key &key)
{
	return TU::DefaultHash<Key>{}(key);
}

template <class Key>
bool equal(const Key &lhs, const Key &rhs)
{
	return TU::DefaultEqual<Key>{}(lhs, rhs);
}

// Whether the hashes of keys, all of them different, are all different.
template <class Key>
bool allHashesDiffer(const std::vector<Key> &keys)
{
	std::vector<size_t> hashes;
	for (const Key &key : keys)
		hashes.push_back(hashOf(key));
	std::sort(hashes.begin(), hashes.end());
	for (size_t i = 1; i < hashes.size(); ++i)
		if (hashes[i] == hashes[i - 1])
			return false;
	return true;
}

// How many different values bits (hash >> shift) & mask take over the hashes
// of the keys 0..count-1.
size_t distinctBits(size_t count, int shift, size_t mask)
{
	std::vector<bool> seen(mask + 1, false);
	size_t            distinct = 0;
	for (uint64_t key = 0; key < count; ++key)
	{
		const size_t bits = (hashOf(key) >> shift) & mask;
		if (!seen[bits])
		{
			seen[bits] = true;
			++distinct;
		}
	}
	return distinct;
}

} // namespace

template <>
struct std::hash<Padded>
{
	size_t operator()(const Padded &key) const
	{
		return std::hash<uint64_t>{}(key.m_id) * 31 + key.m_kind;
	}
};

// ---------------------------------------------------------------------------
// Interface
// ---------------------------------------------------------------------------

// DefaultHash takes a const key and returns a size_t; DefaultEqual takes two
// and returns a bool. Both are empty, so containers pay nothing to hold them.
TEST(FlatHashTest, CallSignatures)
{
	static_assert(HashesTo<TU::DefaultHash<int>, int>);
	static_assert(HashesTo<TU::DefaultHash<uint64_t>, uint64_t>);
	static_assert(HashesTo<TU::DefaultHash<std::string>, std::string>);
	static_assert(Compares<TU::DefaultEqual<int>, int>);
	static_assert(Compares<TU::DefaultEqual<std::string>, std::string>);
	static_assert(std::is_empty_v<TU::DefaultHash<int>>);
	static_assert(std::is_empty_v<TU::DefaultEqual<int>>);
	static_assert(std::is_empty_v<TU::DefaultHash<std::string>>);
	static_assert(std::is_empty_v<TU::DefaultEqual<std::string>>);
}

// Every DefaultHash declares that its results are well mixed.
TEST(FlatHashTest, HashesAreAvalanching)
{
	static_assert(IsAvalanching<TU::DefaultHash<int>>);
	static_assert(IsAvalanching<TU::DefaultHash<int *>>);
	static_assert(IsAvalanching<TU::DefaultHash<std::string>>);
	static_assert(IsAvalanching<TU::DefaultHash<std::string_view>>);
}

// Strings are transparent: a container of std::string finds keys given as
// std::string_view or literals. Other keys are not.
TEST(FlatHashTest, OnlyStringsAreTransparent)
{
	static_assert(IsTransparent<TU::DefaultHash<std::string>>);
	static_assert(IsTransparent<TU::DefaultHash<std::string_view>>);
	static_assert(IsTransparent<TU::DefaultEqual<std::string>>);
	static_assert(IsTransparent<TU::DefaultEqual<std::string_view>>);
	static_assert(HashesTo<TU::DefaultHash<std::string>, std::string_view>);
	static_assert(HashesTo<TU::DefaultHash<std::string>, const char *>);
	static_assert(Compares<TU::DefaultEqual<std::string>, std::string_view>);
	static_assert(!IsTransparent<TU::DefaultHash<int>>);
	static_assert(!IsTransparent<TU::DefaultEqual<int>>);
	static_assert(!IsTransparent<TU::DefaultHash<const char *>>);
}

// ---------------------------------------------------------------------------
// Plain keys
// ---------------------------------------------------------------------------

// The same key always has the same hash, and equal keys are equal.
TEST(FlatHashTest, EqualKeysHaveEqualHashes)
{
	EXPECT_EQ(hashOf(42), hashOf(42));
	EXPECT_EQ(hashOf(uint64_t{7}), hashOf(uint64_t{7}));
	EXPECT_TRUE(equal(42, 42));
	EXPECT_FALSE(equal(42, 43));
	EXPECT_EQ(hashOf(std::string("text")), hashOf(std::string("text")));
}

// Every integer type up to 64 bits is a key, bool and the extremes included,
// and different values have different hashes.
TEST(FlatHashTest, IntegerTypes)
{
	EXPECT_NE(hashOf(false), hashOf(true));
	EXPECT_NE(hashOf(int8_t{-128}), hashOf(int8_t{127}));
	EXPECT_NE(hashOf(uint16_t{0}), hashOf(uint16_t{65535}));
	EXPECT_NE(hashOf(std::numeric_limits<int32_t>::min()),
			  hashOf(std::numeric_limits<int32_t>::max()));
	EXPECT_NE(hashOf(int64_t{-1}), hashOf(int64_t{0}));
	EXPECT_NE(hashOf(uint64_t{0}), hashOf(std::numeric_limits<uint64_t>::max()));
	std::vector<int64_t> keys;
	for (int64_t key = -1000; key <= 1000; ++key)
		keys.push_back(key);
	keys.push_back(std::numeric_limits<int64_t>::min());
	keys.push_back(std::numeric_limits<int64_t>::max());
	EXPECT_TRUE(allHashesDiffer(keys));
}

// Enums are hashed by value: negative values, a small underlying type and a
// 64-bit one.
TEST(FlatHashTest, Enums)
{
	EXPECT_TRUE(allHashesDiffer(std::vector{Level::kLow, Level::kZero, Level::kHigh}));
	EXPECT_TRUE(allHashesDiffer(std::vector{Wide::kSmall, Wide::kHuge}));
	EXPECT_TRUE(equal(Level::kLow, Level::kLow));
	EXPECT_FALSE(equal(Level::kLow, Level::kHigh));
}

// Floating point values are hashed by value: -0.0 and 0.0 are equal, with
// equal hashes, and other values have different hashes.
TEST(FlatHashTest, FloatingPoint)
{
	EXPECT_TRUE(equal(-0.0, 0.0));
	EXPECT_EQ(hashOf(-0.0), hashOf(0.0));
	EXPECT_TRUE(equal(-0.0F, 0.0F));
	EXPECT_EQ(hashOf(-0.0F), hashOf(0.0F));
	EXPECT_FALSE(equal(1.5, 2.5));
	const double infinity = std::numeric_limits<double>::infinity();
	const double smallest = std::numeric_limits<double>::denorm_min();
	EXPECT_TRUE(allHashesDiffer(std::vector<double>{
		0.0, 1.0, -1.0, 0.5, 1e-300, 1e300, infinity, -infinity, smallest}));
	EXPECT_TRUE(allHashesDiffer(std::vector<float>{0.0F, 1.0F, -1.0F, 0.25F, 3e38F}));
}

// Pointers are hashed and compared by address, a const char* too: two
// buffers with the same text are different keys.
TEST(FlatHashTest, PointersAreAddresses)
{
	const int values[3] = {5, 5, 5};
	EXPECT_TRUE(allHashesDiffer(std::vector{&values[0], &values[1], &values[2]}));
	EXPECT_FALSE(equal(&values[0], &values[1]));
	const char  first[]  = "same";
	const char  second[] = "same";
	const char *lhs      = first;
	const char *rhs      = second;
	EXPECT_FALSE(equal(lhs, rhs));
	EXPECT_TRUE(equal(lhs, lhs));
	EXPECT_EQ(hashOf(lhs), hashOf(lhs));
}

// std::string and std::string_view hash their text, the same for both, and
// a literal finds a std::string.
TEST(FlatHashTest, Strings)
{
	const std::string      text = "a string longer than sixteen bytes";
	const std::string_view view = text;
	EXPECT_EQ(hashOf(text), hashOf(view));
	EXPECT_EQ(TU::DefaultHash<std::string>{}("abc"), hashOf(std::string("abc")));
	EXPECT_TRUE(TU::DefaultEqual<std::string>{}(text, view));
	EXPECT_FALSE(equal(std::string("abc"), std::string("abd")));
	EXPECT_TRUE(allHashesDiffer(std::vector<std::string>{"", "a", "b", "ab", "ba", "abc"}));
}

// A key without padding is hashed by its bytes: every member counts.
TEST(FlatHashTest, KeysWithoutPadding)
{
	const EdgeTime key = {1, 2, 3};
	EXPECT_EQ(hashOf(key), hashOf(EdgeTime{1, 2, 3}));
	EXPECT_TRUE(equal(key, EdgeTime{1, 2, 3}));
	EXPECT_FALSE(equal(key, EdgeTime{1, 2, 4}));
	EXPECT_TRUE(allHashesDiffer(std::vector<EdgeTime>{
		{1, 2, 3}, {0, 2, 3}, {1, 0, 3}, {1, 2, 0}, {2, 1, 3}, {3, 2, 1}}));
}

// Other keys with a std::hash use it, whatever the bytes of their padding,
// and their operator==.
TEST(FlatHashTest, KeysWithAStdHash)
{
	Padded lhs;
	Padded rhs;
	std::memset(static_cast<void *>(&lhs), 0x00, sizeof(lhs));
	std::memset(static_cast<void *>(&rhs), 0xFF, sizeof(rhs));
	lhs.m_id = rhs.m_id = 9;
	lhs.m_kind = rhs.m_kind = 2;
	EXPECT_TRUE(equal(lhs, rhs));
	EXPECT_EQ(hashOf(lhs), hashOf(rhs));
	EXPECT_TRUE(allHashesDiffer(std::vector<Padded>{{9, 2}, {9, 3}, {8, 2}}));
}

// The results are well mixed: over consecutive keys, the low 7 bits and the
// high bits take every value, which the containers rely on.
TEST(FlatHashTest, ConsecutiveKeysSpreadOverEveryBit)
{
	EXPECT_EQ(distinctBits(4096, 0, 127), 128U);
	EXPECT_EQ(distinctBits(4096, 57, 127), 128U);
	EXPECT_EQ(distinctBits(65536, 20, 1023), 1024U);
}

// Corner cases: the empty string, and strings that differ only by their
// length or by a zero byte.
TEST(FlatHashTest, CornerCases)
{
	EXPECT_EQ(hashOf(std::string()), hashOf(std::string_view()));
	EXPECT_NE(hashOf(std::string("a")), hashOf(std::string("a\0", 2)));
	std::vector<std::string> lengths;
	for (size_t length = 0; length <= 64; ++length)
		lengths.push_back(std::string(length, 'x'));
	EXPECT_TRUE(allHashesDiffer(lengths));
}

// ---------------------------------------------------------------------------
// Compound keys
// ---------------------------------------------------------------------------

// A struct with hashFields() is hashed by those fields, whatever the bytes of
// its padding, and compared by its operator==.
TEST(FlatHashTest, StructsWithHashFields)
{
	CategoryKey lhs;
	CategoryKey rhs;
	std::memset(static_cast<void *>(&lhs), 0x00, sizeof(lhs));
	std::memset(static_cast<void *>(&rhs), 0xFF, sizeof(rhs));
	lhs.m_category = rhs.m_category = 3;
	lhs.m_time = rhs.m_time = -5;
	lhs.m_realization = rhs.m_realization = 7;
	EXPECT_TRUE(equal(lhs, rhs));
	EXPECT_EQ(hashOf(lhs), hashOf(rhs));
	EXPECT_TRUE(allHashesDiffer(std::vector<CategoryKey>{
		{3, -5, 7}, {4, -5, 7}, {3, 5, 7}, {3, -5, 8}}));
}

// Without operator==, DefaultEqual compares the fields of hashFields(): a
// member left out counts for neither the hash nor the equality.
TEST(FlatHashTest, HashFieldsWithoutOperatorEqual)
{
	const IdAndCache key = {1.5, 2, 100};
	EXPECT_TRUE(equal(key, IdAndCache{1.5, 2, 200}));
	EXPECT_EQ(hashOf(key), hashOf(IdAndCache{1.5, 2, 200}));
	EXPECT_FALSE(equal(key, IdAndCache{1.5, 3, 100}));
	EXPECT_FALSE(equal(key, IdAndCache{2.5, 2, 100}));
}

// With both, DefaultEqual uses operator== and DefaultHash hashFields().
TEST(FlatHashTest, OperatorEqualComesBeforeHashFields)
{
	const Named lhs = {1, "one"};
	const Named rhs = {1, "uno"};
	EXPECT_TRUE(equal(lhs, rhs));
	EXPECT_EQ(hashOf(lhs), hashOf(rhs));
}

// std::pair and std::tuple need no code: every element and their order
// count.
TEST(FlatHashTest, PairsAndTuples)
{
	using Pair = std::pair<int32_t, int32_t>;
	EXPECT_EQ(hashOf(Pair{1, 2}), hashOf(Pair{1, 2}));
	EXPECT_TRUE(equal(Pair{1, 2}, Pair{1, 2}));
	EXPECT_TRUE(allHashesDiffer(std::vector<Pair>{{1, 2}, {2, 1}, {1, 3}, {0, 2}}));
	using Tuple = std::tuple<uint64_t, std::string, int8_t>;
	EXPECT_EQ(hashOf(Tuple{1, "a", 2}), hashOf(Tuple{1, "a", 2}));
	EXPECT_TRUE(allHashesDiffer(std::vector<Tuple>{
		{1, "a", 2}, {0, "a", 2}, {1, "b", 2}, {1, "a", 3}}));
}

// The elements of a pair are hashed and compared as their own types are:
// -0.0 and 0.0 are equal, and an element with hashFields() and no operator==
// works.
TEST(FlatHashTest, ElementsAreHashedAndComparedAsTheirTypes)
{
	using Pair = std::pair<double, int>;
	EXPECT_TRUE(equal(Pair{-0.0, 1}, Pair{0.0, 1}));
	EXPECT_EQ(hashOf(Pair{-0.0, 1}), hashOf(Pair{0.0, 1}));
	using WithFields     = std::pair<IdAndCache, int>;
	const WithFields lhs = {{1.5, 2, 100}, 3};
	const WithFields rhs = {{1.5, 2, 200}, 3};
	EXPECT_TRUE(equal(lhs, rhs));
	EXPECT_EQ(hashOf(lhs), hashOf(rhs));
}

// Arrays and vectors are keys, whatever DefaultHash hashes their elements
// are: integers, floating point values, pointers, pairs, strings, nested
// vectors and structs with hashFields() (whose padding doesn't count).
TEST(FlatHashTest, ArraysAndVectors)
{
	using Ints = std::array<int32_t, 3>;
	EXPECT_TRUE(allHashesDiffer(std::vector<Ints>{{1, 2, 3}, {3, 2, 1}, {1, 2, 4}}));
	using Doubles = std::array<double, 2>;
	EXPECT_TRUE(equal(Doubles{-0.0, 1.0}, Doubles{0.0, 1.0}));
	EXPECT_EQ(hashOf(Doubles{-0.0, 1.0}), hashOf(Doubles{0.0, 1.0}));
	using Vector = std::vector<int64_t>;
	EXPECT_EQ(hashOf(Vector{1, 2, 3}), hashOf(Vector{1, 2, 3}));
	EXPECT_TRUE(allHashesDiffer(std::vector<Vector>{{1, 2, 3}, {3, 2, 1}, {1, 2}}));
	using DoubleVector = std::vector<double>;
	EXPECT_EQ(hashOf(DoubleVector{-0.0}), hashOf(DoubleVector{0.0}));
	using PairVector = std::vector<std::pair<int, int>>;
	EXPECT_TRUE(allHashesDiffer(std::vector<PairVector>{{{1, 2}}, {{2, 1}}, {{1, 2}, {1, 2}}}));
	const int values[2] = {5, 5};
	using Pointers      = std::array<const int *, 2>;
	EXPECT_TRUE(allHashesDiffer(std::vector<Pointers>{
		{&values[0], &values[1]}, {&values[1], &values[0]}}));
	using Strings = std::vector<std::string>;
	EXPECT_TRUE(equal(Strings{"a", "b"}, Strings{"a", "b"}));
	EXPECT_TRUE(allHashesDiffer(std::vector<Strings>{{"a", "b"}, {"b", "a"}, {"ab"}}));
	using Nested = std::vector<std::vector<int>>;
	EXPECT_TRUE(allHashesDiffer(std::vector<Nested>{{{1}, {2}}, {{2}, {1}}, {{1, 2}}}));
	std::array<CategoryKey, 1> lhs;
	std::array<CategoryKey, 1> rhs;
	std::memset(static_cast<void *>(&lhs), 0x00, sizeof(lhs));
	std::memset(static_cast<void *>(&rhs), 0xFF, sizeof(rhs));
	lhs[0].m_category = rhs[0].m_category = 3;
	lhs[0].m_time = rhs[0].m_time = -5;
	lhs[0].m_realization = rhs[0].m_realization = 7;
	EXPECT_TRUE(equal(lhs, rhs));
	EXPECT_EQ(hashOf(lhs), hashOf(rhs));
}

// Corner cases: the length of a vector counts ({} {0} {0, 0} differ, for
// integers, floating point values and bools), and so does a field of value
// zero in a pair.
TEST(FlatHashTest, CompoundCornerCases)
{
	EXPECT_TRUE(allHashesDiffer(std::vector<std::vector<int>>{{}, {0}, {0, 0}}));
	EXPECT_TRUE(allHashesDiffer(std::vector<std::vector<double>>{{}, {0.0}, {0.0, 0.0}}));
	EXPECT_TRUE(allHashesDiffer(std::vector<std::vector<bool>>{{}, {false}, {false, false}, {true}}));
	using Pair = std::pair<uint64_t, uint64_t>;
	EXPECT_TRUE(allHashesDiffer(std::vector<Pair>{{0, 0}, {0, 1}, {1, 0}}));
}
