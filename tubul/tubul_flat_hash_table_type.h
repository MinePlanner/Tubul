#pragma once

// tubul_flat_hash_table_type.h - the types that tubul_flat_hash_map.h and
// tubul_flat_hash_set.h need to declare their interface (namespace
// TU::detail::flathash). Not meant to be included on its own.

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace TU::detail::flathash
{

// A forward iterator over the elements of a table (const when Const is
// true), defined below.
template <class Slot, bool Const>
class TableIterator;

// ---------------------------------------------------------------------------
// Transparent lookup
// ---------------------------------------------------------------------------

template <class Hash, class Equal>
concept IsTransparent = requires
{
	typename Hash::is_transparent;
	typename Equal::is_transparent;
};

// The type of a lookup's key argument: K (deduced) when the container is
// transparent, else Key. Through a class template, not std::conditional,
// which would keep K from being deduced.
template <bool Transparent>
struct KeyArgSelector
{
	template <class K, class Key>
	using Type = Key;
};
template <>
struct KeyArgSelector<true>
{
	template <class K, class Key>
	using Type = K;
};

// Whether K is a key type other than Key that a transparent container
// accepts.
template <class K, class Key, class Hash, class Equal>
concept OtherKey =
	IsTransparent<Hash, Equal> && !std::is_same_v<std::remove_cvref_t<K>, Key>;

// Whether a key argument is not an iterator (so that erase(key) isn't chosen
// for an iterator when the container is transparent).
template <class Arg, class Iterator, class ConstIterator>
concept NotIterator = !std::is_convertible_v<Arg, Iterator> &&
					  !std::is_convertible_v<Arg, ConstIterator>;

} // namespace TU::detail::flathash
