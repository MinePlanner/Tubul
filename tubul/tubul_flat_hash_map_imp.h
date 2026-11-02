#pragma once

// tubul_flat_hash_map_imp.h - the implementation of TU::FlatHashMap,
// declared in tubul_flat_hash_map.h, on the table of
// tubul_flat_hash_table_imp.h. Not meant to be included on its own.

#include <cstddef>
#include <utility>

#include "tubul_flat_hash_map.h"
#include "tubul_flat_hash_table_imp.h"

namespace TU
{

// ---------------------------------------------------------------------------
// Iterators and capacity
// ---------------------------------------------------------------------------

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::begin() noexcept -> iterator
{
	return m_table.begin();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::begin() const noexcept
	-> const_iterator
{
	return m_table.begin();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::cbegin() const noexcept
	-> const_iterator
{
	return m_table.begin();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::end() noexcept -> iterator
{
	return m_table.end();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::end() const noexcept
	-> const_iterator
{
	return m_table.end();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::cend() const noexcept
	-> const_iterator
{
	return m_table.end();
}

template <class Key, class Value, class Hash, class Equal>
bool FlatHashMap<Key, Value, Hash, Equal>::empty() const noexcept
{
	return m_table.size() == 0;
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::size() const noexcept -> size_type
{
	return m_table.size();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::max_size() const noexcept
	-> size_type
{
	return m_table.max_size();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::capacity() const noexcept
	-> size_type
{
	return m_table.capacity();
}

// ---------------------------------------------------------------------------
// Insertion
// ---------------------------------------------------------------------------

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::insert(const value_type &value)
	-> std::pair<iterator, bool>
{
	return m_table.findOrInsert(value.first, value);
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::insert(value_type &&value)
	-> std::pair<iterator, bool>
{
	return m_table.findOrInsert(value.first, std::move(value));
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

template <class Key, class Value, class Hash, class Equal>
template <class K>
auto FlatHashMap<Key, Value, Hash, Equal>::find(const KeyArg<K> &key)
	-> iterator
{
	return m_table.find(key);
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
auto FlatHashMap<Key, Value, Hash, Equal>::find(const KeyArg<K> &key) const
	-> const_iterator
{
	return m_table.find(key);
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
bool FlatHashMap<Key, Value, Hash, Equal>::contains(const KeyArg<K> &key) const
{
	return m_table.contains(key);
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
auto FlatHashMap<Key, Value, Hash, Equal>::count(const KeyArg<K> &key) const
	-> size_type
{
	return m_table.contains(key) ? 1 : 0;
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
auto FlatHashMap<Key, Value, Hash, Equal>::equal_range(const KeyArg<K> &key)
	-> std::pair<iterator, iterator>
{
	return m_table.equal_range(key);
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
auto FlatHashMap<Key, Value, Hash, Equal>::equal_range(
	const KeyArg<K> &key) const -> std::pair<const_iterator, const_iterator>
{
	return m_table.equal_range(key);
}

// ---------------------------------------------------------------------------
// Erasure
// ---------------------------------------------------------------------------

template <class Key, class Value, class Hash, class Equal>
void FlatHashMap<Key, Value, Hash, Equal>::clear() noexcept
{
	m_table.clear();
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::erase(const_iterator pos)
	-> iterator
{
	return m_table.erase(pos);
}

template <class Key, class Value, class Hash, class Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::erase(const_iterator first,
												 const_iterator last)
	-> iterator
{
	return m_table.erase(first, last);
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
auto FlatHashMap<Key, Value, Hash, Equal>::erase(const KeyArg<K> &key)
	-> size_type
	requires detail::flathash::NotIterator<KeyArg<K>, iterator, const_iterator>
{
	return m_table.eraseKey(key);
}

} // namespace TU
