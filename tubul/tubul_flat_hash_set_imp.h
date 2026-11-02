#pragma once

// tubul_flat_hash_set_imp.h - the implementation of TU::FlatHashSet,
// declared in tubul_flat_hash_set.h, on the table of
// tubul_flat_hash_table_imp.h. Not meant to be included on its own.

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

#include "tubul_flat_hash_set.h"
#include "tubul_flat_hash_table_imp.h"

namespace TU
{

// ---------------------------------------------------------------------------
// Iterators and capacity
// ---------------------------------------------------------------------------

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::begin() const noexcept -> iterator
{
	return m_table.begin();
}

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::end() const noexcept -> iterator
{
	return m_table.end();
}

template <class Key, class Hash, class Equal>
bool FlatHashSet<Key, Hash, Equal>::empty() const noexcept
{
	return m_table.size() == 0;
}

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::size() const noexcept -> size_type
{
	return m_table.size();
}

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::capacity() const noexcept -> size_type
{
	return m_table.capacity();
}

} // namespace TU
