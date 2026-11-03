#pragma once

// tubul_flat_hash_imp.h - the implementation of tubul_flat_hash.h (namespace
// TU::detail::flathash), included at its end. Not meant to be included on
// its own.
//
// Every hash is made of one operation, the folded multiply (below), applied
// to 64-bit words:
// - a key that is one number (integer, enum, pointer) is one word, mixed;
// - a key made of fields (hashFields(), std::pair, std::tuple, std::array)
//   turns each field into a word and hashes the words as bytes;
// - bytes (strings, keys without padding) are hashed 16 at a time, with the
//   length in the seed.

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

namespace TU::detail::flathash
{

// ---------------------------------------------------------------------------
// Mixing
// ---------------------------------------------------------------------------

// Odd constants with well spread bits (those of wyhash).
inline constexpr uint64_t kMultiplier = 0x9E3779B97F4A7C15ULL;
inline constexpr uint64_t kSecret     = 0xA0761D6478BD642FULL;

// The 128-bit product of a and b, with its two halves xor-ed: every bit of
// the result depends on every bit of a and of b.
inline uint64_t foldedMultiply(uint64_t a, uint64_t b)
{
#ifdef __SIZEOF_INT128__
	const unsigned __int128 product = static_cast<unsigned __int128>(a) * b;
	return static_cast<uint64_t>(product) ^ static_cast<uint64_t>(product >> 64);
#elif defined(_MSC_VER)
	uint64_t       high = 0;
	const uint64_t low  = _umul128(a, b, &high);
	return low ^ high;
#else
	// Four 32x32-bit products, added as in school.
	const uint64_t aLow    = a & 0xFFFFFFFFU;
	const uint64_t aHigh   = a >> 32;
	const uint64_t bLow    = b & 0xFFFFFFFFU;
	const uint64_t bHigh   = b >> 32;
	const uint64_t lowLow  = aLow * bLow;
	const uint64_t middle1 = aHigh * bLow + (lowLow >> 32);
	const uint64_t middle2 = aLow * bHigh + (middle1 & 0xFFFFFFFFU);
	const uint64_t high    = aHigh * bHigh + (middle1 >> 32) + (middle2 >> 32);
	const uint64_t low     = (middle2 << 32) | (lowLow & 0xFFFFFFFFU);
	return low ^ high;
#endif
}

// One word, mixed. (No seed is xor-ed in first: it made lookups of integer
// keys 5-10% slower, by lengthening the chain to the probe.)
inline uint64_t mix(uint64_t word)
{
	return foldedMultiply(word, kMultiplier);
}

// ---------------------------------------------------------------------------
// Bytes
// ---------------------------------------------------------------------------

inline uint64_t load64(const unsigned char *bytes)
{
	uint64_t word = 0;
	std::memcpy(&word, bytes, sizeof(word));
	return word;
}

inline uint64_t load32(const unsigned char *bytes)
{
	uint32_t word = 0;
	std::memcpy(&word, bytes, sizeof(word));
	return word;
}

// The hash of size bytes: one folded multiply per 16 bytes, chained, with the
// length in the seed, and a last mix (without it, the high bits of the last
// words barely change the result). The last 1 to 16 bytes are read as two
// words that may overlap (which is fine: the length tells how they overlap).
inline uint64_t hashBytes(const void *data, size_t size)
{
	const auto *bytes = static_cast<const unsigned char *>(data);
	uint64_t    hash  = mix(size);
	while (size > 16)
	{
		hash = foldedMultiply(hash ^ load64(bytes), load64(bytes + 8) ^ kSecret);
		bytes += 16;
		size -= 16;
	}
	uint64_t first  = 0;
	uint64_t second = 0;
	if (size > 8)
	{
		first  = load64(bytes);
		second = load64(bytes + size - 8);
	}
	else if (size >= 4)
	{
		first  = load32(bytes);
		second = load32(bytes + size - 4);
	}
	else if (size > 0)
	{
		first = (uint64_t{bytes[0]} << 16) | (uint64_t{bytes[size / 2]} << 8) |
				bytes[size - 1];
	}
	return mix(foldedMultiply(hash ^ first, second ^ kSecret));
}

// ---------------------------------------------------------------------------
// Kinds of keys
// ---------------------------------------------------------------------------

template <class T>
concept HasHashFields = requires(const T &key)
{ hashFields(key); };

template <class T>
struct IsPairOrTuple : std::false_type
{
};
template <class First, class Second>
struct IsPairOrTuple<std::pair<First, Second>> : std::true_type
{
};
template <class... Elements>
struct IsPairOrTuple<std::tuple<Elements...>> : std::true_type
{
};

template <class T>
struct IsStdArray : std::false_type
{
};
template <class Element, size_t Size>
struct IsStdArray<std::array<Element, Size>> : std::true_type
{
};

template <class T>
struct IsStdVector : std::false_type
{
};
template <class Element, class Allocator>
struct IsStdVector<std::vector<Element, Allocator>> : std::true_type
{
};

template <class T>
struct IsStringView : std::false_type
{
};
template <class Char, class Traits>
struct IsStringView<std::basic_string_view<Char, Traits>> : std::true_type
{
};

template <class T>
concept HasStdHash = requires(const T &key)
{
	{ std::hash<T>{}(key) } -> std::convertible_to<size_t>;
};

// A number, enum or pointer as one word: integers and enums by value,
// pointers by address.
template <class T>
uint64_t numberWord(const T &number)
{
	if constexpr (std::is_pointer_v<T>)
		return reinterpret_cast<uintptr_t>(number);
	else
		return static_cast<uint64_t>(number);
}

// A field as one word: numbers, enums and pointers as they are (a floating
// point value by its bits, with -0.0 as 0.0 since they are equal), anything
// else by its DefaultHash.
template <class T>
uint64_t fieldWord(const T &field)
{
	if constexpr (std::is_integral_v<T> || std::is_enum_v<T> ||
				  std::is_pointer_v<T>)
		return numberWord(field);
	else if constexpr (std::is_floating_point_v<T>)
	{
		const double value = field;
		return value == 0 ? 0 : std::bit_cast<uint64_t>(value);
	}
	else
		return DefaultHash<T>{}(field);
}

// The hash of the elements of a std::pair, std::tuple or std::array: one word
// per element, hashed as bytes.
template <class TupleLike>
uint64_t hashElements(const TupleLike &elements)
{
	return std::apply(
		[](const auto &...element)
		{
			const std::array<uint64_t, sizeof...(element)> words = {
				fieldWord(element)...};
			return hashBytes(words.data(), words.size() * sizeof(uint64_t));
		},
		elements);
}

// The hash of a std::vector: its bytes when its elements have no padding,
// else one word per element, chained, with the length in the seed
// (std::vector<bool> has no bytes to read).
template <class Vector>
uint64_t hashVector(const Vector &vector)
{
	using Element = Vector::value_type;
	if constexpr (std::has_unique_object_representations_v<Element> &&
				  !std::is_same_v<Element, bool>)
		return hashBytes(static_cast<const void *>(vector.data()),
						 vector.size() * sizeof(Element));
	else
	{
		uint64_t hash = mix(vector.size());
		for (const auto &element : vector)
			hash = foldedMultiply(hash ^ fieldWord<Element>(element), kMultiplier);
		return mix(hash);
	}
}

// Whether two pairs or tuples have equal elements, each compared with the
// DefaultEqual of its type.
template <class TupleLike>
bool equalElements(const TupleLike &lhs, const TupleLike &rhs)
{
	return [&]<size_t... Index>(std::index_sequence<Index...>)
	{
		return (DefaultEqual<std::remove_cvref_t<
					std::tuple_element_t<Index, TupleLike>>>{}(
					std::get<Index>(lhs), std::get<Index>(rhs)) &&
				...);
	}(std::make_index_sequence<std::tuple_size_v<TupleLike>>{});
}

template <class>
inline constexpr bool kAlwaysFalse = false;

} // namespace TU::detail::flathash

namespace TU
{

inline size_t DefaultHash<std::string>::operator()(std::string_view text) const
{
	return detail::flathash::hashBytes(text.data(), text.size());
}

inline bool DefaultEqual<std::string>::operator()(std::string_view lhs,
												  std::string_view rhs) const
{
	return lhs == rhs;
}

template <class Key>
size_t DefaultHash<Key>::operator()(const Key &key) const
{
	namespace fh = detail::flathash;
	if constexpr ((std::is_integral_v<Key> || std::is_enum_v<Key> ||
				   std::is_pointer_v<Key>) &&
				  sizeof(Key) <= sizeof(uint64_t))
		return fh::mix(fh::numberWord(key));
	else if constexpr (std::is_floating_point_v<Key>)
		// By its bits, as a field: std::hash differs between libraries.
		return fh::mix(fh::fieldWord(key));
	else if constexpr (fh::HasHashFields<Key>)
		return fh::hashElements(hashFields(key));
	else if constexpr (fh::IsPairOrTuple<Key>::value ||
					   (fh::IsStdArray<Key>::value &&
						!std::has_unique_object_representations_v<Key>))
		// (An array without padding is hashed by its bytes, below.)
		return fh::hashElements(key);
	else if constexpr (fh::IsStringView<Key>::value)
		// Its bytes are a pointer and a length, but it compares the text.
		return fh::hashBytes(key.data(), key.size() * sizeof(key[0]));
	else if constexpr (std::has_unique_object_representations_v<Key>)
		return fh::hashBytes(&key, sizeof(Key));
	else if constexpr (fh::IsStdVector<Key>::value)
		return fh::hashVector(key);
	else if constexpr (fh::HasStdHash<Key>)
		return fh::mix(std::hash<Key>{}(key));
	else
	{
		static_assert(fh::kAlwaysFalse<Key>,
					  "DefaultHash can't hash this key: declare the fields that make up its "
					  "identity, e.g. `friend auto hashFields(const Key& key) { return "
					  "std::tie(key.m_a, key.m_b); }`, or pass a Hash to the container");
		return 0;
	}
}

template <class Key>
bool DefaultEqual<Key>::operator()(const Key &lhs, const Key &rhs) const
{
	namespace fh = detail::flathash;
	if constexpr (fh::IsPairOrTuple<Key>::value)
		return fh::equalElements(lhs, rhs);
	else if constexpr (std::equality_comparable<Key>)
		return lhs == rhs;
	else if constexpr (fh::HasHashFields<Key>)
		return fh::equalElements(hashFields(lhs), hashFields(rhs));
	else
	{
		static_assert(std::has_unique_object_representations_v<Key>,
					  "DefaultEqual can't compare this key: give it an operator== or "
					  "hashFields()");
		return std::memcmp(&lhs, &rhs, sizeof(Key)) == 0;
	}
}

} // namespace TU
