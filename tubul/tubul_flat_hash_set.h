#pragma once

// tubul_flat_hash_set.h - TU::FlatHashSet, an open-addressing hash set with
// the interface of std::unordered_set, on the same table as TU::FlatHashMap
// (tubul_flat_hash_map.h). The DESCRIPTION, ITERATION ORDER and THREAD
// SAFETY of tubul_flat_hash_map.h apply to it as they are; what differs is
// below.
//
// ITERATOR INVALIDATION
//   Operations                                   Invalidated
//   read-only operations, swap                   never
//   clear, rehash, reserve, operator=            always
//   insert, emplace                              every iterator, pointer and
//                                                  reference, only if the
//                                                  table has no room for a
//                                                  new key
//   erase                                        only the erased key
//   Unlike std::unordered_set, a rehash moves the keys, so it invalidates
//   pointers and references too. Inserting a key that is already there never
//   invalidates anything. After reserve(n), inserting until size() is n
//   invalidates nothing, as long as nothing is erased in between.
//
// TEMPLATE PARAMETERS
//   Key     the type of the keys.
//   Hash    the hash of a key, DefaultHash<Key> by default (see
//           tubul_flat_hash.h); mixed once more unless it declares
//           `using is_avalanching = void;` (DefaultHash does).
//   Equal   the equality of keys, DefaultEqual<Key> by default.
//   There is no Allocator parameter.
//
// DIFFERENCES WITH std::unordered_set
//   - Iterators, pointers and references: see ITERATOR INVALIDATION.
//   - A rehash moves the keys, so they needn't be copyable; if moving a key
//     can throw, an exception while rehashing leaves the set broken.
//   - No bucket interface, node handles (extract, merge), allocators,
//     emplace_hint() or insertions with a hint.
//   - The maximum load factor is fixed at 7/8: max_load_factor(float) is
//     accepted and ignored, as in absl::flat_hash_set.
//   - Code written for absl::flat_hash_set compiles unchanged as long as it
//     uses none of the above and its keys don't rely on AbslHashValue (a
//     struct key declares hashFields() instead, see tubul_flat_hash.h).
//   - Preconditions (dereferencing or erasing end(), an iterator to an erased
//     key) are only checked by assert.
//
// STRING KEYS
//   A FlatHashSet<std::string> is searched with std::string_view,
//   std::string or string literals, without building a std::string:
//   find(), contains(), count(), erase() and equal_range() accept any key
//   type K that Hash and Equal take when both declare
//   `using is_transparent = void;` (as DefaultHash and DefaultEqual do for
//   strings).
//
// BENCHMARKS
//   Against absl::flat_hash_set and std::unordered_set, and against
//   TU::FlatSet and std::set, for uint64_t keys: see the end of this file.
//
// EXAMPLE
//   TU::FlatHashSet<int64_t> seen;
//   for (int64_t id : ids)
//       if (!seen.insert(id).second) report(id); // a duplicate
//   if (seen.contains(42)) use(42);

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "tubul_flat_hash.h"
#include "tubul_flat_hash_set_type.h"

namespace TU
{

template <class Key, class Hash = DefaultHash<Key>, class Equal = DefaultEqual<Key>>
class FlatHashSet
{
	// A lookup's key: any K for a transparent set, else Key.
	template <class K>
	using KeyArg = typename detail::flathash::KeyArgSelector<
		detail::flathash::IsTransparent<Hash, Equal>>::template Type<K, Key>;

public:
	using key_type        = Key;
	using value_type      = Key;
	using size_type       = size_t;
	using difference_type = std::ptrdiff_t;
	using hasher          = Hash;
	using key_equal       = Equal;
	using reference       = value_type &;
	using const_reference = const value_type &;
	using pointer         = value_type *;
	using const_pointer   = const value_type *;
	// Both are constant: changing a key would break the table.
	using iterator       = detail::flathash::TableIterator<Key, true>;
	using const_iterator = detail::flathash::TableIterator<Key, true>;
	// Only for generic code that names it: there is no allocator support.
	using allocator_type = std::allocator<value_type>;

	// Constructors: empty, without memory; with room for capacity keys; from
	// a range or an initializer list of keys. Copies copy the keys; a
	// moved-from set is empty, without memory.
	FlatHashSet() = default;
	explicit FlatHashSet(size_type capacity, const Hash &hash = Hash(), const Equal &equal = Equal());
	template <std::input_iterator InputIt>
	FlatHashSet(InputIt first, InputIt last, size_type capacity = 0, const Hash &hash = Hash(), const Equal &equal = Equal());
	FlatHashSet(std::initializer_list<value_type> init, size_type capacity = 0, const Hash &hash = Hash(), const Equal &equal = Equal());
	FlatHashSet &operator=(std::initializer_list<value_type> init);

	// Iterators, over the keys in no particular order.
	iterator       begin() const noexcept;
	const_iterator cbegin() const noexcept;
	iterator       end() const noexcept;
	const_iterator cend() const noexcept;

	// Capacity. capacity() is the number of slots.
	bool      empty() const noexcept;
	size_type size() const noexcept;
	size_type max_size() const noexcept;
	size_type capacity() const noexcept;

	// Erases every key, keeping the memory.
	void clear() noexcept;

	// Inserts key unless it is there. Returns the key in the set and whether
	// it was inserted.
	std::pair<iterator, bool> insert(const Key &key);
	std::pair<iterator, bool> insert(Key &&key);
	template <std::input_iterator InputIt>
	void insert(InputIt first, InputIt last);
	void insert(std::initializer_list<value_type> init);

	// Inserts the key built from args unless it is there.
	template <class... Args>
	std::pair<iterator, bool> emplace(Args &&...args);

	// Erases a key given by an iterator, a range or a key. The first two
	// return the iterator after the last erased key; the last returns how
	// many it erased (0 or 1).
	iterator erase(const_iterator pos);
	iterator erase(const_iterator first, const_iterator last);
	template <class K = Key>
	size_type erase(const KeyArg<K> &key)
		requires detail::flathash::NotIterator<KeyArg<K>, iterator, const_iterator>;

	void swap(FlatHashSet &other) noexcept(
		std::is_nothrow_swappable_v<Hash> && std::is_nothrow_swappable_v<Equal>);

	// Lookups. find() returns end() if key isn't there; equal_range() an
	// empty range then, else the range of its key.
	template <class K = Key>
	iterator find(const KeyArg<K> &key) const;
	template <class K = Key>
	bool contains(const KeyArg<K> &key) const;
	template <class K = Key>
	size_type count(const KeyArg<K> &key) const;
	template <class K = Key>
	std::pair<iterator, iterator> equal_range(const KeyArg<K> &key) const;

	// Hash policy, as in FlatHashMap.
	float     load_factor() const noexcept;
	float     max_load_factor() const noexcept;
	void      max_load_factor(float ignored) noexcept;
	size_type bucket_count() const noexcept;
	void      rehash(size_type capacity);
	void      reserve(size_type count);

	hasher    hash_function() const;
	key_equal key_eq() const;
};

// From the keys of a range or of an initializer list, then the capacity,
// Hash and Equal, as in the constructors.
template <std::input_iterator InputIt,
		  class Hash  = DefaultHash<std::iter_value_t<InputIt>>,
		  class Equal = DefaultEqual<std::iter_value_t<InputIt>>>
FlatHashSet(InputIt, InputIt, size_t = 0, Hash = Hash(), Equal = Equal())
	-> FlatHashSet<std::iter_value_t<InputIt>, Hash, Equal>;

template <class Key, class Hash = DefaultHash<Key>, class Equal = DefaultEqual<Key>>
FlatHashSet(std::initializer_list<Key>, size_t = 0, Hash = Hash(), Equal = Equal()) -> FlatHashSet<Key, Hash, Equal>;

// Erases the keys for which pred(key) is true, and returns how many.
template <class Key, class Hash, class Equal, class Pred>
size_t erase_if(FlatHashSet<Key, Hash, Equal> &set, Pred pred);

// Sets are equal when they have the same keys.
template <class Key, class Hash, class Equal>
bool operator==(const FlatHashSet<Key, Hash, Equal> &lhs,
				const FlatHashSet<Key, Hash, Equal> &rhs);

template <class Key, class Hash, class Equal>
void swap(FlatHashSet<Key, Hash, Equal> &lhs,
		  FlatHashSet<Key, Hash, Equal> &rhs) noexcept(noexcept(lhs.swap(rhs)));

} // namespace TU

// -----------------------------------------------------------------------------
// Benchmarks of uint64_t keys (2026-10-29)
// -----------------------------------------------------------------------------
//
// Three sets of uint64_t: TU::FlatHashSet with DefaultHash,
// absl::flat_hash_set (Abseil 20250127.1, absl::Hash) and std::unordered_set
// (std::hash, libstdc++ 14), with 1K to 10M keys.
// Nanoseconds per operation, the median of 7 runs of each benchmark; the
// operations, "max variation" and the rows marked (r) are those of the
// uint64_t table of tubul_flat_hash_map.h.
//
// TU::FlatHashSet / absl::flat_hash_set (above 1 is slower): find of a present
// key 0.98-1.09, of a missing key 0.88-1.08, inserts while growing 0.97-1.41
// and after reserve() 0.85-1.05, erase or insert 0.74-0.96 (faster), iterate
// 0.80-1.95 (slower in small tables). std::unordered_set is slower at
// everything but finding the keys 0..n-1 and iterating small tables.
//
// The tables are wider than 80 columns; keep clang-format from rewrapping them.
// clang-format off
//   operation              size  TU::FlatHashSet  absl::flat_hash_set  std::unordered_set  max variation
//   find, key present        1K             1.75                 1.79                2.62             2%
//                            4K             1.94                 1.87                2.63             2%
//                           64K             3.24                 2.96                9.16             1%
//                            1M            12.72                12.35               28.99             2%
//                            4M            18.78                18.82               32.57             9%
//                           10M            23.94                23.02               35.40             2%
//   find, keys 0..n-1        1K             1.64                 1.73                1.59             3%
//                            4K             1.90                 1.80                1.68             4%
//                           64K             3.28                 2.95                2.96             1%
//                            1M            12.65                12.99               17.52             3%
//                            4M            18.92                19.14               21.76             6%
//                           10M            23.71                23.44               22.75             2%
//   find, key missing        1K             1.59                 1.62                3.47             2%
//                            4K             1.70                 1.74                3.25             1%
//                           64K             2.35                 2.17               13.14             1%
//                            1M             3.51                 3.93               30.76             6%
//                            4M             5.66                 6.46               34.36             4%
//                           10M            11.89                11.23               39.36             2%
//   insert, set growing      1K            16.15                11.47               40.49             3%
//                            4K            17.65                14.18               44.29             2%
//                           64K            19.36                15.74               58.35             1%
//                            1M            25.32                24.65              226.13             9%
//                            4M            32.30                32.67              347.03             6%
//                           10M            36.92                38.10              373.24             3%
//   insert, after reserve    1K             4.23                 4.97               34.99             3%
//                            4K             4.22                 4.95               37.15             2%
//                           64K             5.30                 6.09               45.27             2%
//                            1M            15.37                14.89              138.82             2%
//                            4M            24.26                23.19              199.28             8%
//                           10M            33.66                34.97              212.88             6%
//   erase or insert *        1K             5.08                 6.90               23.04             9%
//                            4K             6.44                 8.12               24.78             5%
//                           64K             8.28                10.19               41.65             1%
//                            1M            22.79                25.24              192.75             4% (r)
//                            4M            35.94                41.83              235.09             4%
//                           10M            50.90                53.19              261.09             3%
//   iterate, per key         1K             2.90                 1.49                1.25             9% (r)
//                            4K             2.91                 1.52                2.15             4%
//                           64K             2.92                 3.51                5.59             1%
//                            1M             2.93                 3.66               40.79             3%
//                            4M             2.93                 3.66               59.06             1%
//                           10M             2.91                 2.92               62.46             0%
// clang-format on
// Setup: as for the uint64_t table of tubul_flat_hash_map.h, at 4.39-4.54 GHz.

// -----------------------------------------------------------------------------
// Against TU::FlatSet and std::set (2026-11-02)
// -----------------------------------------------------------------------------
//
// TU::FlatSet (a sorted vector), std::set, TU::FlatHashSet and
// std::unordered_set of uint64_t, random keys. Nanoseconds per operation, the
// median of 9 runs of the four containers in a rotating order; insert one by
// one starts from an empty container, build from a range uses the range
// constructor. Measured while the machine was busy (load average about 129 on
// 16 hardware threads, every CPU busy), pinned to one CPU and timed in the CPU
// time of the thread, so that waiting to be scheduled isn't counted; the times
// are about twice those of an idle machine (the other hardware thread of the
// core and the caches are shared), but a second run gave the same times within
// 15%, and the ratios hold.
//
// TU::FlatHashSet is faster than std::set at everything: 8-24 times to find a
// present key, 8-64 times a missing one, 2.6-4.2 times to insert or build, and
// 1.2-27 times to iterate. Against TU::FlatSet (above 1: TU::FlatHashSet is
// faster): find of a present key 6.3-12, of a missing key 7-17, insert one by
// one 2.1 at 1K and 53 at 64K (not run at 1M), build from a range 0.49-1.1
// and iterate 0.04-0.15: TU::FlatSet is the better choice for a set built
// once and then mostly iterated.
//
// clang-format off
//   operation              size      TU::FlatSet         std::set  TU::FlatHashSet    unordered_set
//   find, key present         1K            29.29            37.76             4.60             6.97
//                            64K           100.82           162.30             8.18            17.57
//                             1M           183.15           697.85            29.22            53.79
//   find, key missing         1K            32.88            35.87             4.60            10.39
//                            64K           100.37           160.28             5.74            20.61
//                             1M           183.53           674.44            10.53            59.31
//   iterate, per element      1K             0.70             5.58             4.63             2.72
//                            64K             0.15            18.72             4.11             8.29
//                             1M             0.36           108.09             4.03            53.07
//   build from a range        1K            31.53           103.53            39.39            65.13
//                            64K            57.53           216.89            52.10           112.20
//                             1M            72.30           606.69           148.84           341.03
//   insert one by one         1K            83.35           105.33            40.43            65.22
//                            64K          2733.90           215.52            51.82           112.09
//                             1M              n/a           587.30           142.98           331.13
// clang-format on
// Setup: as for the table against TU::FlatMap of tubul_flat_hash_map.h.
