#pragma once

// tubul_flat_hash_set_type.h - the types that tubul_flat_hash_set.h needs to
// declare its interface (namespace TU::detail::flathash). Not meant to be
// included on its own.

#include <memory>
#include <utility>

#include "tubul_flat_hash_table_type.h"

namespace TU::detail::flathash
{

// What the slots of a FlatHashSet's table hold: a key, which users only see
// as const.
template <class KeyType>
struct SetPolicy
{
	using Key                              = KeyType;
	using Slot                             = Key;
	static constexpr bool kMutableIterator = false;

	// Only called with slots of the table, never with temporaries.
	static const Key &key(const Slot &slot)
	{
		return slot;
	}

	// Moves a key to another slot when the table is rehashed (from is
	// destroyed right after).
	static void relocate(Slot *to, Slot &from)
	{
		std::construct_at(to, std::move(from));
	}
};

} // namespace TU::detail::flathash
