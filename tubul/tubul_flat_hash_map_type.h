#pragma once

// tubul_flat_hash_map_type.h - the types that tubul_flat_hash_map.h needs to
// declare its interface (namespace TU::detail::flathash). Not meant to be
// included on its own.

#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "tubul_flat_hash_table_type.h"

namespace TU::detail::flathash
{

// The key and value types of a range of pairs (deduction guides).
template <std::input_iterator InputIt>
using IterKey =
	std::remove_const_t<typename std::iter_value_t<InputIt>::first_type>;
template <std::input_iterator InputIt>
using IterMapped = std::iter_value_t<InputIt>::second_type;

// What the slots of a FlatHashMap's table hold: a std::pair<const Key,
// Value>.
template <class KeyType, class Value>
struct MapPolicy
{
	using Key                              = KeyType;
	using Slot                             = std::pair<const Key, Value>;
	static constexpr bool kMutableIterator = true;

	static const Key &key(const Slot &slot)
	{
		return slot.first;
	}

	// Moves an element to another slot when the table is rehashed (from is
	// destroyed right after). The key is const for users, but is moved here,
	// so that keys needn't be copyable.
	//
	// TODO: moving from from.first through const_cast is formally undefined
	// behavior: the slot was built as a std::pair<const Key, Value>, so its
	// key is a const object. It works for now because the moved-from key is
	// only destroyed afterwards, and compilers don't rely on a const member
	// reached through a pointer keeping its value; libstdc++'s node handles
	// (extract().key()) do the same. Abseil avoids it with a slot that is a
	// union of std::pair<const Key, Value> and std::pair<Key, Value>, which
	// would replace Slot here.
	static void relocate(Slot *to, Slot &from)
	{
		std::construct_at(to, std::move(const_cast<Key &>(from.first)), std::move(from.second));
	}
};

} // namespace TU::detail::flathash
