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
auto FlatHashSet<Key, Hash, Equal>::cbegin() const noexcept -> const_iterator
{
	return m_table.begin();
}

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::end() const noexcept -> iterator
{
	return m_table.end();
}

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::cend() const noexcept -> const_iterator
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

// ---------------------------------------------------------------------------
// Insertion and erasure
// ---------------------------------------------------------------------------

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::insert(const Key &key)
	-> std::pair<iterator, bool>
{
	return m_table.findOrInsert(key, key);
}

template <class Key, class Hash, class Equal>
auto FlatHashSet<Key, Hash, Equal>::insert(Key &&key)
	-> std::pair<iterator, bool>
{
	return m_table.findOrInsert(key, std::move(key));
}

template <class Key, class Hash, class Equal>
template <std::input_iterator InputIt>
void FlatHashSet<Key, Hash, Equal>::insert(InputIt first, InputIt last)
{
	for (; first != last; ++first)
		emplace(*first);
}

template <class Key, class Hash, class Equal>
void FlatHashSet<Key, Hash, Equal>::insert(std::initializer_list<value_type> init)
{
	insert(init.begin(), init.end());
}

// A Key is inserted as it is; anything else builds the key first.
template <class Key, class Hash, class Equal>
template <class... Args>
auto FlatHashSet<Key, Hash, Equal>::emplace(Args &&...args)
	-> std::pair<iterator, bool>
{
	if constexpr (sizeof...(Args) == 1 &&
				  (std::is_same_v<std::remove_cvref_t<Args>, Key> && ...))
		return insert(std::forward<Args>(args)...);
	else
		return insert(Key(std::forward<Args>(args)...));
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

template <class Key, class Hash, class Equal>
template <class K>
auto FlatHashSet<Key, Hash, Equal>::find(const KeyArg<K> &key) const -> iterator
{
	return m_table.find(key);
}

template <class Key, class Hash, class Equal>
template <class K>
bool FlatHashSet<Key, Hash, Equal>::contains(const KeyArg<K> &key) const
{
	return m_table.contains(key);
}

template <class Key, class Hash, class Equal>
template <class K>
auto FlatHashSet<Key, Hash, Equal>::count(const KeyArg<K> &key) const
	-> size_type
{
	return m_table.contains(key) ? 1 : 0;
}

} // namespace TU
