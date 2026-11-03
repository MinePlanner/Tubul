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
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
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
// After the last slot, where iteration stops; lookups never read it.
inline constexpr uint8_t kSentinel = 0xFF;

constexpr bool isFull(uint8_t ctrl)
{
	return ctrl < 0x80;
}

// The sentinels after the last slot: enough for a read of 8 bytes that starts
// at the first one.
inline constexpr size_t kCtrlPadding = 8;

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
auto TableIterator<Slot, Const>::operator++() -> TableIterator &
{
	assert(m_ctrl != nullptr && isFull(*m_ctrl));
	const size_t distance = 1 + distanceToFull(m_ctrl + 1);
	m_ctrl += distance;
	m_slot += distance;
	return *this;
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
size_t Table<Policy, Hash, Equal>::capacity() const noexcept
{
	return m_capacity;
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

// ---------------------------------------------------------------------------
// Table: memory
// ---------------------------------------------------------------------------

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
