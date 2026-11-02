#pragma once

// tubul_flat_hash_table_type.h - the types that tubul_flat_hash_map.h and
// tubul_flat_hash_set.h need to declare their interface (namespace
// TU::detail::flathash). Not meant to be included on its own.

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

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

// ---------------------------------------------------------------------------
// Iterator
// ---------------------------------------------------------------------------

// A forward iterator over the elements of a table (const when Const is
// true): a pointer to the control byte of its slot and one to the slot, moved
// together. ++ skips the slots that aren't full and stops at the sentinel
// after the last slot, which is end().
template <class Slot, bool Const>
class TableIterator
{
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type        = std::remove_const_t<Slot>;
	using difference_type   = std::ptrdiff_t;
	using reference         = std::conditional_t<Const, const Slot &, Slot &>;
	using pointer           = std::conditional_t<Const, const Slot *, Slot *>;

	TableIterator() = default;
	// iterator converts to const_iterator.
	template <bool OtherConst>
		requires(Const && !OtherConst)
	TableIterator(const TableIterator<Slot, OtherConst> &other);

	reference      operator*() const;
	pointer        operator->() const;
	TableIterator &operator++();
	TableIterator  operator++(int);

	friend bool operator==(const TableIterator &lhs, const TableIterator &rhs)
	{
		return lhs.m_ctrl == rhs.m_ctrl;
	}

private:
	template <class, class, class>
	friend class Table;
	friend class TableIterator<Slot, true>;

	TableIterator(const uint8_t *ctrl, Slot *slot);

	const uint8_t *m_ctrl = nullptr;
	Slot          *m_slot = nullptr;
};

// ---------------------------------------------------------------------------
// Table
// ---------------------------------------------------------------------------

// The table under both containers (see DESCRIPTION in tubul_flat_hash_map.h).
// A Policy gives the type of its slots (Slot) and the key of a slot
// (Policy::key()). The containers hold one and build their interface on it.
template <class Policy, class Hash, class Equal>
class Table
{
public:
	using Key            = typename Policy::Key;
	using Slot           = typename Policy::Slot;
	using iterator       = TableIterator<Slot, !Policy::kMutableIterator>;
	using const_iterator = TableIterator<Slot, true>;

	Table() noexcept;
	Table(const Table &other);
	Table(Table &&other) noexcept(std::is_nothrow_move_constructible_v<Hash> &&
								  std::is_nothrow_move_constructible_v<Equal>);
	Table &operator=(const Table &other);
	Table &operator=(Table &&other) noexcept(
		std::is_nothrow_move_assignable_v<Hash> &&
		std::is_nothrow_move_assignable_v<Equal>);
	~Table();

	iterator       begin() noexcept;
	const_iterator begin() const noexcept;
	iterator       end() noexcept;
	const_iterator end() const noexcept;

	size_t size() const noexcept;
	size_t max_size() const noexcept;
	size_t capacity() const noexcept;

	template <class K>
	iterator find(const K &key);
	template <class K>
	const_iterator find(const K &key) const;
	template <class K>
	bool contains(const K &key) const;

	template <class K>
	std::pair<iterator, iterator> equal_range(const K &key);
	template <class K>
	std::pair<const_iterator, const_iterator> equal_range(const K &key) const;

	// Erases every element, keeping the memory.
	void clear() noexcept;
	// Erases an element or a range, and returns the iterator after it.
	iterator erase(const_iterator pos);
	iterator erase(const_iterator first, const_iterator last);
	// Erases key if it is there, and returns how many elements it erased.
	template <class K>
	size_t eraseKey(const K &key);

	float load_factor() const noexcept;
	float max_load_factor() const noexcept;
	// Rebuilds the table with at least capacity slots, or as few as the
	// elements need; rehash(0) on an empty table frees its memory. Throws
	// std::length_error if capacity is above max_size().
	void rehash(size_t capacity);
	// Makes room for count elements without rehashing. Throws
	// std::length_error if count is above max_size().
	void reserve(size_t count);

	// Finds key, or inserts the slot built from args (which must hold key).
	// Returns the element and whether it was inserted.
	template <class K, class... Args>
	std::pair<iterator, bool> findOrInsert(const K &key, Args &&...args);

private:
	struct ProbeResult
	{
		bool   m_found;
		size_t m_index;
	};

	template <class K>
	size_t findIndex(const K &key) const;
	template <class K>
	ProbeResult findOrFree(const K &key, size_t hash) const;
	template <class K>
	size_t         hashOf(const K &key) const;
	static uint8_t tagOf(size_t hash);
	iterator       iteratorAt(size_t index) const;
	size_t         indexOf(const_iterator pos) const;
	void           eraseAt(size_t index);
	iterator       firstFrom(size_t index) const;
	size_t         emptySlotFor(size_t hash) const;
	void           rehashTo(size_t capacity);
	void           destroyElements() noexcept;
	void           destroyAndFree() noexcept;
	static void    deallocate(Slot *slots, const uint8_t *ctrl, size_t capacity) noexcept;

	Slot                       *m_slots;
	uint8_t                    *m_ctrl;
	size_t                      m_capacity;
	size_t                      m_size;
	size_t                      m_tombstones;
	[[no_unique_address]] Hash  m_hash;
	[[no_unique_address]] Equal m_equal;
};

} // namespace TU::detail::flathash
