#pragma once

// tubul_flat_hash_table_imp.h - the implementation of the table under
// TU::FlatHashMap and TU::FlatHashSet (namespace TU::detail::flathash),
// declared in tubul_flat_hash_table_type.h. Not meant to be included on its
// own.
//
// A Swiss table in its simplest form:
// - The elements are in one array of slots. A second array has one control
//   byte per slot: kEmpty, kDeleted (a tombstone left by erase) or, for a
//   full slot, its tag: the low 7 bits of the hash (full slots are the bytes
//   without the high bit).
// - The slots are in groups of 8 (slots 0-7, 8-15, ...), whose 8 control
//   bytes are read as one 64-bit word and tested all at once (Group).
// - A key's probe starts at group (hash >> 7) mod the number of groups and
//   goes on one group at a time (wrapping around) until it finds the key or a
//   group with an empty slot. Only slots whose tag matches are compared with
//   the key. A new key goes to the first empty or deleted slot of its probe.
// - The capacity is 0 or a power of two, at least kMinCapacity (one group).
//   At most 7/8 of the slots are full or deleted, so that every probe ends.
//   An insertion that would go over doubles the table, or, when the elements
//   fit in half of those slots, rehashes it at the same capacity.
// - After the last slot, kCtrlPadding bytes of kSentinel stop iteration,
//   which looks for the bytes that are full or the sentinel, 8 at a time.
// Elements never move except when the table is rehashed, which is what the
// iterator invalidation rules of the containers need.
//
// Reading the control bytes one at a time instead of by groups is simpler but
// much slower (3.6 times for missing keys, 1.8 times for present keys, in
// tables of 64K keys): each byte is a branch that the processor can't
// predict.

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "tubul_flat_hash.h"
#include "tubul_flat_hash_table_type.h"

namespace TU::detail::flathash
{

// ---------------------------------------------------------------------------
// Control bytes and sizes
// ---------------------------------------------------------------------------

// A full slot's byte is 7 bits of the hash, 0x00 to 0x7F; the others have the
// high bit set.
inline constexpr uint8_t kEmpty   = 0x80;
inline constexpr uint8_t kDeleted = 0xFE;
// After the last slot, where iteration stops; lookups never read it.
inline constexpr uint8_t kSentinel = 0xFF;

constexpr bool isFull(uint8_t ctrl)
{
	return ctrl < 0x80;
}

// The sentinels after the last slot: enough for a read of 8 bytes that starts
// at the first one.
inline constexpr size_t kCtrlPadding = 8;

inline constexpr size_t kGroupSize   = 8;
inline constexpr size_t kMinCapacity = kGroupSize;

// The table grows when more than this fraction of its slots would be full or
// deleted.
inline constexpr size_t kMaxLoadNumerator   = 7;
inline constexpr size_t kMaxLoadDenominator = 8;

// How many slots of a table of capacity slots may be full or deleted.
constexpr size_t maxUsed(size_t capacity)
{
	return capacity / kMaxLoadDenominator * kMaxLoadNumerator;
}

// The smallest capacity that holds count elements.
constexpr size_t capacityFor(size_t count)
{
	size_t capacity = kMinCapacity;
	while (maxUsed(capacity) < count)
		capacity *= 2;
	return capacity;
}

// The control bytes of a table without slots: only the sentinels, so that
// begin() == end(). Never written.
inline uint8_t gEmptyCtrl[kCtrlPadding] = {kSentinel, kSentinel, kSentinel, kSentinel, kSentinel, kSentinel, kSentinel, kSentinel};

// ---------------------------------------------------------------------------
// Groups: 8 control bytes at once
// ---------------------------------------------------------------------------

// 8 control bytes read as one word, byte i of memory in byte i of the word
// (bits 8i to 8i + 7), whatever the byte order of the machine.
inline uint64_t loadCtrlWord(const uint8_t *ctrl)
{
	uint64_t word = 0;
	if constexpr (std::endian::native == std::endian::little)
		std::memcpy(&word, ctrl, sizeof(word));
	else
		for (size_t i = 0; i < 8; ++i)
			word |= uint64_t{ctrl[i]} << (8 * i);
	return word;
}

// The bytes of a group selected by a test are given as a mask with the high
// bit of each selected byte set; this is the position (0-7) of the first.
inline size_t firstByte(uint64_t mask)
{
	return static_cast<size_t>(std::countr_zero(mask)) / 8;
}

// The tests on the 8 control bytes of a group, with plain arithmetic on a
// 64-bit word (SIMD within a register).
class Group
{
public:
	explicit Group(const uint8_t *ctrl) : m_word(loadCtrlWord(ctrl))
	{
	}

	// The bytes equal to tag: those of word ^ (tag in every byte) that are
	// zero, by the classic test. A byte just above a match may be reported too
	// (the subtraction borrows into it), but it is then tag ^ 1, another full
	// slot, whose key is compared for nothing.
	uint64_t match(uint8_t tag) const
	{
		const uint64_t diff = m_word ^ (kLows * tag);
		return (diff - kLows) & ~diff & kHighs;
	}
	// The empty bytes (0x80): high bit set, bit 1 clear (the shift by 6 moves
	// bit 1 of each byte to its bit 7). Exact.
	uint64_t empty() const
	{
		return m_word & ~(m_word << 6) & kHighs;
	}
	// The empty or deleted bytes (0x80, 0xFE): high bit set, bit 0 clear.
	// Exact.
	uint64_t free() const
	{
		return m_word & ~(m_word << 7) & kHighs;
	}
	// The full bytes and the sentinel: all but the empty or deleted ones.
	uint64_t fullOrSentinel() const
	{
		return ~free() & kHighs;
	}

private:
	static constexpr uint64_t kLows  = 0x0101010101010101ULL;
	static constexpr uint64_t kHighs = 0x8080808080808080ULL;

	uint64_t m_word;
};

// How many control bytes there are from ctrl to the first full slot or
// sentinel, read 8 at a time (not necessarily a group).
inline size_t distanceToFull(const uint8_t *ctrl)
{
	for (size_t distance = 0;; distance += 8)
	{
		const uint64_t found = Group(ctrl + distance).fullOrSentinel();
		if (found != 0)
			return distance + firstByte(found);
	}
}

// ---------------------------------------------------------------------------
// TableIterator
// ---------------------------------------------------------------------------

template <class Slot, bool Const>
template <bool OtherConst>
	requires(Const && !OtherConst)
TableIterator<Slot, Const>::TableIterator(
	const TableIterator<Slot, OtherConst> &other) : m_ctrl(other.m_ctrl), m_slot(other.m_slot)
{
}

template <class Slot, bool Const>
TableIterator<Slot, Const>::TableIterator(const uint8_t *ctrl, Slot *slot) : m_ctrl(ctrl), m_slot(slot)
{
}

template <class Slot, bool Const>
auto TableIterator<Slot, Const>::operator*() const -> reference
{
	assert(m_ctrl != nullptr && isFull(*m_ctrl));
	return *m_slot;
}

template <class Slot, bool Const>
auto TableIterator<Slot, Const>::operator->() const -> pointer
{
	return &**this;
}

template <class Slot, bool Const>
auto TableIterator<Slot, Const>::operator++() -> TableIterator &
{
	assert(m_ctrl != nullptr && isFull(*m_ctrl));
	const size_t distance = 1 + distanceToFull(m_ctrl + 1);
	m_ctrl += distance;
	m_slot += distance;
	return *this;
}

template <class Slot, bool Const>
auto TableIterator<Slot, Const>::operator++(int) -> TableIterator
{
	TableIterator old = *this;
	++*this;
	return old;
}

// ---------------------------------------------------------------------------
// Table: construction and iteration
// ---------------------------------------------------------------------------

template <class Policy, class Hash, class Equal>
Table<Policy, Hash, Equal>::Table() noexcept
	: m_slots(nullptr), m_ctrl(gEmptyCtrl), m_capacity(0), m_size(0),
	  m_tombstones(0)
{
}

template <class Policy, class Hash, class Equal>
Table<Policy, Hash, Equal>::~Table()
{
	destroyAndFree();
}

template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::begin() noexcept -> iterator
{
	return firstFrom(0);
}

template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::begin() const noexcept -> const_iterator
{
	return firstFrom(0);
}

template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::end() noexcept -> iterator
{
	return iteratorAt(m_capacity);
}

template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::end() const noexcept -> const_iterator
{
	return iteratorAt(m_capacity);
}

template <class Policy, class Hash, class Equal>
size_t Table<Policy, Hash, Equal>::size() const noexcept
{
	return m_size;
}

template <class Policy, class Hash, class Equal>
size_t Table<Policy, Hash, Equal>::max_size() const noexcept
{
	return static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max()) /
		   (sizeof(Slot) + 1);
}

template <class Policy, class Hash, class Equal>
size_t Table<Policy, Hash, Equal>::capacity() const noexcept
{
	return m_capacity;
}

// ---------------------------------------------------------------------------
// Table: lookup and insertion
// ---------------------------------------------------------------------------

template <class Policy, class Hash, class Equal>
template <class K>
auto Table<Policy, Hash, Equal>::find(const K &key) -> iterator
{
	return iteratorAt(findIndex(key));
}

template <class Policy, class Hash, class Equal>
template <class K>
auto Table<Policy, Hash, Equal>::find(const K &key) const -> const_iterator
{
	return iteratorAt(findIndex(key));
}

template <class Policy, class Hash, class Equal>
template <class K>
bool Table<Policy, Hash, Equal>::contains(const K &key) const
{
	return findIndex(key) != m_capacity;
}

template <class Policy, class Hash, class Equal>
template <class K, class... Args>
auto Table<Policy, Hash, Equal>::findOrInsert(const K &key, Args &&...args)
	-> std::pair<iterator, bool>
{
	const size_t hash  = hashOf(key);
	size_t       index = 0;
	if (m_capacity != 0)
	{
		const ProbeResult probe = findOrFree(key, hash);
		if (probe.m_found)
			return {iteratorAt(probe.m_index), false};
		index = probe.m_index;
	}
	const bool reusesTombstone = m_capacity != 0 && m_ctrl[index] == kDeleted;
	if (!reusesTombstone && m_size + m_tombstones + 1 > maxUsed(m_capacity))
	{
		// No room: rehash. The element is built first, so that an exception
		// leaves the table as it was, and since args may refer to an element
		// of this table, which rehashing moves.
		Slot element(std::forward<Args>(args)...);
		if (m_capacity == 0)
			rehashTo(kMinCapacity);
		else if (m_size + 1 <= maxUsed(m_capacity) / 2)
			rehashTo(m_capacity);
		else
			rehashTo(m_capacity * 2);
		index = emptySlotFor(hash);
		Policy::relocate(&m_slots[index], element);
	}
	else
	{
		std::construct_at(&m_slots[index], std::forward<Args>(args)...);
		if (reusesTombstone)
			--m_tombstones;
	}
	m_ctrl[index] = tagOf(hash);
	++m_size;
	return {iteratorAt(index), true};
}

// The index of key's slot, or m_capacity if it isn't there.
template <class Policy, class Hash, class Equal>
template <class K>
size_t Table<Policy, Hash, Equal>::findIndex(const K &key) const
{
	if (m_size == 0)
		return m_capacity;
	const size_t  hash      = hashOf(key);
	const uint8_t tag       = tagOf(hash);
	const size_t  groupMask = m_capacity / kGroupSize - 1;
	for (size_t group = (hash >> 7) & groupMask;; group = (group + 1) & groupMask)
	{
		const size_t first = group * kGroupSize;
		const Group  ctrl(m_ctrl + first);
		for (uint64_t match = ctrl.match(tag); match != 0; match &= match - 1)
		{
			const size_t index = first + firstByte(match);
			if (m_equal(Policy::key(m_slots[index]), key))
				return index;
		}
		if (ctrl.empty() != 0)
			return m_capacity;
	}
}

// The slot of key if it is there, else the first empty or deleted slot of its
// probe, where it goes. The table must have slots.
template <class Policy, class Hash, class Equal>
template <class K>
auto Table<Policy, Hash, Equal>::findOrFree(const K &key, size_t hash) const
	-> ProbeResult
{
	const uint8_t tag       = tagOf(hash);
	size_t        free      = m_capacity;
	const size_t  groupMask = m_capacity / kGroupSize - 1;
	for (size_t group = (hash >> 7) & groupMask;; group = (group + 1) & groupMask)
	{
		const size_t first = group * kGroupSize;
		const Group  ctrl(m_ctrl + first);
		for (uint64_t match = ctrl.match(tag); match != 0; match &= match - 1)
		{
			const size_t index = first + firstByte(match);
			if (m_equal(Policy::key(m_slots[index]), key))
				return {true, index};
		}
		if (free == m_capacity && ctrl.free() != 0)
			free = first + firstByte(ctrl.free());
		if (ctrl.empty() != 0)
			return {false, free};
	}
}

template <class Policy, class Hash, class Equal>
template <class K>
size_t Table<Policy, Hash, Equal>::hashOf(const K &key) const
{
	// The tag and the start of the probe use different bits of the hash, so
	// all of them must be good: mix a hash that doesn't promise it.
	if constexpr (requires
				  { typename Hash::is_avalanching; })
		return m_hash(key);
	else
		return static_cast<size_t>(mix(m_hash(key)));
}

template <class Policy, class Hash, class Equal>
uint8_t Table<Policy, Hash, Equal>::tagOf(size_t hash)
{
	return static_cast<uint8_t>(hash & 0x7F);
}

template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::iteratorAt(size_t index) const -> iterator
{
	return iterator(m_ctrl + index, m_slots + index);
}

// The first element at index or after it (end() if none).
template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::firstFrom(size_t index) const -> iterator
{
	return iteratorAt(index + distanceToFull(m_ctrl + index));
}

// The slot where a new element of that hash goes, in a table without
// tombstones (just rehashed).
template <class Policy, class Hash, class Equal>
size_t Table<Policy, Hash, Equal>::emptySlotFor(size_t hash) const
{
	const size_t groupMask = m_capacity / kGroupSize - 1;
	for (size_t group = (hash >> 7) & groupMask;; group = (group + 1) & groupMask)
	{
		const size_t   first = group * kGroupSize;
		const uint64_t empty = Group(m_ctrl + first).empty();
		if (empty != 0)
			return first + firstByte(empty);
	}
}

// ---------------------------------------------------------------------------
// Table: erasure and size policy
// ---------------------------------------------------------------------------

template <class Policy, class Hash, class Equal>
template <class K>
auto Table<Policy, Hash, Equal>::equal_range(const K &key)
	-> std::pair<iterator, iterator>
{
	const size_t index = findIndex(key);
	if (index == m_capacity)
		return {end(), end()};
	iterator next = iteratorAt(index);
	++next;
	return {iteratorAt(index), next};
}

template <class Policy, class Hash, class Equal>
template <class K>
auto Table<Policy, Hash, Equal>::equal_range(const K &key) const
	-> std::pair<const_iterator, const_iterator>
{
	const size_t index = findIndex(key);
	if (index == m_capacity)
		return {end(), end()};
	const_iterator next = iteratorAt(index);
	++next;
	return {iteratorAt(index), next};
}

template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::clear() noexcept
{
	destroyElements();
	if (m_capacity != 0)
		std::memset(m_ctrl, kEmpty, m_capacity);
	m_size       = 0;
	m_tombstones = 0;
}

template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::erase(const_iterator pos) -> iterator
{
	const size_t index = indexOf(pos);
	iterator     next  = iteratorAt(index);
	++next;
	eraseAt(index);
	return next;
}

template <class Policy, class Hash, class Equal>
auto Table<Policy, Hash, Equal>::erase(const_iterator first,
									   const_iterator last) -> iterator
{
	while (first != last)
		first = erase(first);
	return iteratorAt(indexOf(last));
}

template <class Policy, class Hash, class Equal>
template <class K>
size_t Table<Policy, Hash, Equal>::eraseKey(const K &key)
{
	const size_t index = findIndex(key);
	if (index == m_capacity)
		return 0;
	eraseAt(index);
	return 1;
}

template <class Policy, class Hash, class Equal>
float Table<Policy, Hash, Equal>::load_factor() const noexcept
{
	if (m_capacity == 0)
		return 0.0F;
	return static_cast<float>(m_size) / static_cast<float>(m_capacity);
}

template <class Policy, class Hash, class Equal>
float Table<Policy, Hash, Equal>::max_load_factor() const noexcept
{
	return static_cast<float>(kMaxLoadNumerator) /
		   static_cast<float>(kMaxLoadDenominator);
}

template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::rehash(size_t capacity)
{
	// (Above max_size(), the capacity would overflow.)
	if (capacity > max_size())
		throw std::length_error("TU::FlatHashMap/FlatHashSet: rehash() above max_size()");
	if (capacity == 0 && m_size == 0)
	{
		destroyAndFree();
		m_slots      = nullptr;
		m_ctrl       = gEmptyCtrl;
		m_capacity   = 0;
		m_tombstones = 0;
		return;
	}
	rehashTo(std::max(std::bit_ceil(capacity), capacityFor(m_size)));
}

template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::reserve(size_t count)
{
	if (count > max_size())
		throw std::length_error("TU::FlatHashMap/FlatHashSet: reserve() above max_size()");
	if (count + m_tombstones > maxUsed(m_capacity))
		rehashTo(capacityFor(count));
}

template <class Policy, class Hash, class Equal>
size_t Table<Policy, Hash, Equal>::indexOf(const_iterator pos) const
{
	assert(pos.m_ctrl >= m_ctrl && pos.m_ctrl <= m_ctrl + m_capacity);
	return static_cast<size_t>(pos.m_ctrl - m_ctrl);
}

template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::eraseAt(size_t index)
{
	std::destroy_at(&m_slots[index]);
	--m_size;
	// No probe goes on past a group with an empty slot, so if this one has
	// one, no probe needs this slot. (A group only gets an empty slot from
	// here or a rehash: one that a probe went past never does.)
	const size_t first = index / kGroupSize * kGroupSize;
	if (Group(m_ctrl + first).empty() != 0)
		m_ctrl[index] = kEmpty;
	else
	{
		m_ctrl[index] = kDeleted;
		++m_tombstones;
	}
}

// ---------------------------------------------------------------------------
// Table: memory
// ---------------------------------------------------------------------------

// Moves every element to a new table of capacity slots.
template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::rehashTo(size_t capacity)
{
	// Allocated first, so that a std::bad_alloc leaves the table as it was.
	Slot *const slots = std::allocator<Slot>().allocate(capacity);
	uint8_t    *ctrl  = nullptr;
	try
	{
		ctrl = new uint8_t[capacity + kCtrlPadding];
	}
	catch (...)
	{
		std::allocator<Slot>().deallocate(slots, capacity);
		throw;
	}
	Slot *const          oldSlots    = std::exchange(m_slots, slots);
	const uint8_t *const oldCtrl     = std::exchange(m_ctrl, ctrl);
	const size_t         oldCapacity = m_capacity;
	std::memset(m_ctrl, kEmpty, capacity);
	std::memset(m_ctrl + capacity, kSentinel, kCtrlPadding);
	m_capacity   = capacity;
	m_tombstones = 0;
	for (size_t old = 0; old < oldCapacity; ++old)
	{
		if (!isFull(oldCtrl[old]))
			continue;
		const size_t hash  = hashOf(Policy::key(oldSlots[old]));
		const size_t index = emptySlotFor(hash);
		Policy::relocate(&m_slots[index], oldSlots[old]);
		std::destroy_at(&oldSlots[old]);
		m_ctrl[index] = tagOf(hash);
	}
	deallocate(oldSlots, oldCtrl, oldCapacity);
}

template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::destroyElements() noexcept
{
	if constexpr (!std::is_trivially_destructible_v<Slot>)
		for (size_t index = 0; index < m_capacity; ++index)
			if (isFull(m_ctrl[index]))
				std::destroy_at(&m_slots[index]);
}

template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::destroyAndFree() noexcept
{
	destroyElements();
	deallocate(m_slots, m_ctrl, m_capacity);
}

template <class Policy, class Hash, class Equal>
void Table<Policy, Hash, Equal>::deallocate(Slot *slots, const uint8_t *ctrl, size_t capacity) noexcept
{
	if (capacity == 0)
		return;
	std::allocator<Slot>().deallocate(slots, capacity);
	delete[] ctrl;
}

} // namespace TU::detail::flathash
