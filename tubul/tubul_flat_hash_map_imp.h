#pragma once

// tubul_flat_hash_map_imp.h - the implementation of TU::FlatHashMap,
// declared in tubul_flat_hash_map.h, on the table of
// tubul_flat_hash_table_imp.h. Not meant to be included on its own.

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tubul_flat_hash_map.h"
#include "tubul_flat_hash_table_imp.h"

namespace TU::detail::flathash
{

template <class... Args>
struct FirstArg
{
	using Type = void;
};
template <class First, class... Rest>
struct FirstArg<First, Rest...>
{
	using Type = std::remove_cvref_t<First>;
};

// emplace(key, value): the arguments are a Key and one more.
template <class Key, class... Args>
concept KeyAndValue =
	sizeof...(Args) == 2 && std::is_same_v<typename FirstArg<Args...>::Type, Key>;

// emplace(pair): the argument is a pair whose first is a Key.
template <class Key, class... Args>
concept PairOfKey =
	sizeof...(Args) == 1 &&
	requires
	{ typename FirstArg<Args...>::Type::first_type; } &&
	std::is_same_v<std::remove_const_t<typename FirstArg<Args...>::Type::first_type>, Key>;

// Inserts key with the value built from args unless key is there. The key is
// only converted to a Key (and the value built) when inserted: findOrInsert()
// looks key up before it builds the element from the references that
// forward_as_tuple() keeps.
template <class Table, class K, class... Args>
auto tryEmplace(Table &table, K &&key, Args &&...args)
{
	return table.findOrInsert(key, std::piecewise_construct, std::forward_as_tuple(std::forward<K>(key)), std::forward_as_tuple(std::forward<Args>(args)...));
}

// Inserts the pair's key with its value unless the key is there, forwarding
// each member of the pair once.
template <class Table, class Pair>
auto emplacePair(Table &table, Pair &&pair)
{
	return tryEmplace(table, std::forward<Pair>(pair).first, std::forward<Pair>(pair).second);
}

// Inserts key with the value obj, or assigns obj to its value, with one
// probe. obj is forwarded twice, but only used once: tryEmplace() leaves its
// arguments untouched when key is there.
template <class Table, class K, class M>
auto insertOrAssign(Table &table, K &&key, M &&obj)
{
	auto result = tryEmplace(table, std::forward<K>(key), std::forward<M>(obj));
	if (!result.second)
		result.first->second = std::forward<M>(obj);
	return result;
}

} // namespace TU::detail::flathash

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

template <class Key, class Value, class Hash, class Equal>
template <std::input_iterator InputIt>
void FlatHashMap<Key, Value, Hash, Equal>::insert(InputIt first, InputIt last)
{
	for (; first != last; ++first)
		emplace(*first);
}

template <class Key, class Value, class Hash, class Equal>
void FlatHashMap<Key, Value, Hash, Equal>::insert(
	std::initializer_list<value_type> init)
{
	insert(init.begin(), init.end());
}

template <class Key, class Value, class Hash, class Equal>
template <class M>
auto FlatHashMap<Key, Value, Hash, Equal>::insert_or_assign(const Key &key,
															M        &&obj)
	-> std::pair<iterator, bool>
{
	return detail::flathash::insertOrAssign(m_table, key, std::forward<M>(obj));
}

template <class Key, class Value, class Hash, class Equal>
template <class M>
auto FlatHashMap<Key, Value, Hash, Equal>::insert_or_assign(Key &&key, M &&obj)
	-> std::pair<iterator, bool>
{
	return detail::flathash::insertOrAssign(m_table, std::move(key), std::forward<M>(obj));
}

template <class Key, class Value, class Hash, class Equal>
template <class K, class M>
	requires detail::flathash::OtherKey<K, Key, Hash, Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::insert_or_assign(K &&key, M &&obj)
	-> std::pair<iterator, bool>
{
	return detail::flathash::insertOrAssign(m_table, std::forward<K>(key), std::forward<M>(obj));
}

// A key and a value, or a pair of them, go to try_emplace(), so that the
// value is only built if the key isn't there; anything else builds the
// element first.
template <class Key, class Value, class Hash, class Equal>
template <class... Args>
auto FlatHashMap<Key, Value, Hash, Equal>::emplace(Args &&...args)
	-> std::pair<iterator, bool>
{
	if constexpr (detail::flathash::KeyAndValue<Key, Args...>)
		return try_emplace(std::forward<Args>(args)...);
	else if constexpr (detail::flathash::PairOfKey<Key, Args...>)
		return detail::flathash::emplacePair(m_table, std::forward<Args>(args)...);
	else
	{
		value_type element(std::forward<Args>(args)...);
		return m_table.findOrInsert(element.first, std::move(element));
	}
}

template <class Key, class Value, class Hash, class Equal>
template <class... Args>
auto FlatHashMap<Key, Value, Hash, Equal>::try_emplace(const Key &key,
													   Args &&...args)
	-> std::pair<iterator, bool>
{
	return detail::flathash::tryEmplace(m_table, key, std::forward<Args>(args)...);
}

template <class Key, class Value, class Hash, class Equal>
template <class... Args>
auto FlatHashMap<Key, Value, Hash, Equal>::try_emplace(Key &&key,
													   Args &&...args)
	-> std::pair<iterator, bool>
{
	return detail::flathash::tryEmplace(m_table, std::move(key), std::forward<Args>(args)...);
}

template <class Key, class Value, class Hash, class Equal>
template <class K, class... Args>
	requires detail::flathash::OtherKey<K, Key, Hash, Equal>
auto FlatHashMap<Key, Value, Hash, Equal>::try_emplace(K &&key, Args &&...args)
	-> std::pair<iterator, bool>
{
	return detail::flathash::tryEmplace(m_table, std::forward<K>(key), std::forward<Args>(args)...);
}

// ---------------------------------------------------------------------------
// Element access
// ---------------------------------------------------------------------------

template <class Key, class Value, class Hash, class Equal>
template <class K>
Value &FlatHashMap<Key, Value, Hash, Equal>::at(const KeyArg<K> &key)
{
	const auto it = m_table.find(key);
	if (it == m_table.end())
		throw std::out_of_range("FlatHashMap::at: no such key");
	return it->second;
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
const Value &FlatHashMap<Key, Value, Hash, Equal>::at(
	const KeyArg<K> &key) const
{
	const auto it = m_table.find(key);
	if (it == m_table.end())
		throw std::out_of_range("FlatHashMap::at: no such key");
	return it->second;
}

template <class Key, class Value, class Hash, class Equal>
Value &FlatHashMap<Key, Value, Hash, Equal>::operator[](const Key &key)
{
	return try_emplace(key).first->second;
}

template <class Key, class Value, class Hash, class Equal>
Value &FlatHashMap<Key, Value, Hash, Equal>::operator[](Key &&key)
{
	return try_emplace(std::move(key)).first->second;
}

template <class Key, class Value, class Hash, class Equal>
template <class K>
	requires detail::flathash::OtherKey<K, Key, Hash, Equal>
Value &FlatHashMap<Key, Value, Hash, Equal>::operator[](K &&key)
{
	return try_emplace(std::forward<K>(key)).first->second;
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
