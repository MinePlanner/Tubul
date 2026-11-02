#pragma once

// tubul_flat_hash_map.h - TU::FlatHashMap, an open-addressing hash map with
// the interface of std::unordered_map, for keys that are plain old data
// (integers, enums, pointers, small structs) above all. TU::FlatHashSet
// (tubul_flat_hash_set.h) is the set on the same table.
//
// DESCRIPTION
//   A "Swiss table": the elements are in one array of slots, without nodes,
//   and a second array has one control byte per slot (empty, deleted, or 7
//   bits of the element's hash). A lookup reads the control bytes of a group
//   of 8 slots at once, compares the key only with the slots whose 7 bits
//   match, and stops at the first group with an empty slot. The capacity is
//   0 or a power of two, at least 8. An insertion that would leave more than
//   7/8 of the slots full or deleted doubles the table, or, when erasures
//   left many deleted slots, rehashes it at the same capacity.
//
// ITERATOR INVALIDATION
//   Operations                                   Invalidated
//   read-only operations, swap                   never
//   clear, rehash, reserve, operator=            always
//   insert, emplace, try_emplace,                every iterator, pointer and
//     insert_or_assign, operator[]                 reference, only if the
//                                                  table has no room for a
//                                                  new key (see below)
//   erase                                        only the erased element
//   Unlike std::unordered_map, a rehash moves the elements, so it invalidates
//   pointers and references too. Inserting a key that is already there never
//   invalidates anything. After reserve(n), inserting until size() is n
//   invalidates nothing, as long as nothing is erased in between.
//
// TEMPLATE PARAMETERS
//   Key, Value  the types of the keys and of the mapped values.
//   Hash        the hash of a key, DefaultHash<Key> by default (see
//               tubul_flat_hash.h). The map uses every bit of the hash, so it
//               mixes the result once more unless Hash declares
//               `using is_avalanching = void;` (DefaultHash does).
//   Equal       the equality of keys, DefaultEqual<Key> by default.
//   There is no Allocator parameter.
//
// DIFFERENCES WITH std::unordered_map
//   - Iterators, pointers and references: see ITERATOR INVALIDATION.
//   - value_type is std::pair<const Key, Value>. A rehash moves the keys and
//     the values, so keys needn't be copyable; if moving a key or a value can
//     throw, an exception while rehashing leaves the map broken.
//   - No bucket interface, node handles (extract, merge), allocators,
//     emplace_hint() or insertions with a hint: a hint can't help where the
//     position of an element only depends on its hash.
//   - The maximum load factor is fixed at 7/8: max_load_factor(float) is
//     accepted and ignored, as in absl::flat_hash_map.
//   - Code written for absl::flat_hash_map compiles unchanged as long as it
//     uses none of the above and its keys don't rely on AbslHashValue (a
//     struct key declares hashFields() instead, see tubul_flat_hash.h).
//   - Preconditions (dereferencing or erasing end(), an iterator to an erased
//     element) are only checked by assert.
//
// ITERATION ORDER
//   Reproducible: two maps that get the same operations in the same order,
//   with the same hash values, iterate in the same order, in any run of the
//   same build, wherever they are in memory. A hash that differs between runs
//   (pointer keys, hashed by their address) gives a different order in every
//   run: sort the elements first when the results must not depend on it.
//
// STRING KEYS
//   A FlatHashMap<std::string, V> is searched with std::string_view,
//   std::string or string literals, and nothing is built from the key unless
//   it is inserted:
//     TU::FlatHashMap<std::string, int> counts;
//     for (std::string_view word : words) ++counts[word];
//     if (auto it = counts.find("the"); it != counts.end()) use(it->second);
//   When both Hash and Equal declare `using is_transparent = void;` (as
//   DefaultHash and DefaultEqual do for strings), find(), contains(),
//   count(), erase(), equal_range(), at(), operator[], try_emplace() and
//   insert_or_assign() accept any key type K that Hash and Equal take.
//
// BENCHMARKS
//   Against absl::flat_hash_map and std::unordered_map, for uint64_t and
//   std::string keys, and against TU::FlatMap and std::map: see the end of
//   this file.
//
// THREAD SAFETY
//   As for standard containers: const member functions may run at the same
//   time, and different maps may be used in different threads at the same
//   time; any other call needs exclusive access to the map.
//
// EXAMPLE
//   TU::FlatHashMap<int64_t, double> prices;
//   prices[42] = 9.5;
//   prices.try_emplace(7, 3.25);
//   if (auto it = prices.find(42); it != prices.end()) total += it->second;
//   for (const auto &[id, price] : prices) print(id, price);
//   prices.erase(7);

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "tubul_flat_hash.h"
#include "tubul_flat_hash_map_type.h"

namespace TU
{

template <class Key, class Value, class Hash = DefaultHash<Key>, class Equal = DefaultEqual<Key>>
class FlatHashMap
{
	// A lookup's key: any K for a transparent map, else Key.
	template <class K>
	using KeyArg = typename detail::flathash::KeyArgSelector<
		detail::flathash::IsTransparent<Hash, Equal>>::template Type<K, Key>;

public:
	using key_type        = Key;
	using mapped_type     = Value;
	using value_type      = std::pair<const Key, Value>;
	using size_type       = size_t;
	using difference_type = std::ptrdiff_t;
	using hasher          = Hash;
	using key_equal       = Equal;
	using reference       = value_type &;
	using const_reference = const value_type &;
	using pointer         = value_type *;
	using const_pointer   = const value_type *;
	using iterator        = detail::flathash::TableIterator<value_type, false>;
	using const_iterator  = detail::flathash::TableIterator<value_type, true>;
	// Only for generic code that names it: there is no allocator support.
	using allocator_type = std::allocator<value_type>;

	// Constructors: empty, without memory; with room for capacity elements;
	// from a range or an initializer list of pairs (the first of equal keys
	// wins). Copies copy the elements; a moved-from map is empty, without
	// memory.
	FlatHashMap() = default;
	explicit FlatHashMap(size_type capacity, const Hash &hash = Hash(), const Equal &equal = Equal());
	template <std::input_iterator InputIt>
	FlatHashMap(InputIt first, InputIt last, size_type capacity = 0, const Hash &hash = Hash(), const Equal &equal = Equal());
	FlatHashMap(std::initializer_list<value_type> init, size_type capacity = 0, const Hash &hash = Hash(), const Equal &equal = Equal());
	FlatHashMap &operator=(std::initializer_list<value_type> init);

	// Iterators, over the elements in no particular order (see ITERATION
	// ORDER).
	iterator       begin() noexcept;
	const_iterator begin() const noexcept;
	const_iterator cbegin() const noexcept;
	iterator       end() noexcept;
	const_iterator end() const noexcept;
	const_iterator cend() const noexcept;

	// Capacity. capacity() is the number of slots.
	bool      empty() const noexcept;
	size_type size() const noexcept;
	size_type max_size() const noexcept;
	size_type capacity() const noexcept;

	// Erases every element, keeping the memory.
	void clear() noexcept;

	// Inserts value unless its key is there. Returns the element with that
	// key and whether it was inserted.
	std::pair<iterator, bool> insert(const value_type &value);
	std::pair<iterator, bool> insert(value_type &&value);
	template <std::input_iterator InputIt>
	void insert(InputIt first, InputIt last);
	void insert(std::initializer_list<value_type> init);

	// Inserts key with the value obj, or assigns obj to its value if key is
	// there. Returns the element and whether it was inserted.
	template <class M>
	std::pair<iterator, bool> insert_or_assign(const Key &key, M &&obj);
	template <class M>
	std::pair<iterator, bool> insert_or_assign(Key &&key, M &&obj);
	template <class K, class M>
		requires detail::flathash::OtherKey<K, Key, Hash, Equal>
	std::pair<iterator, bool> insert_or_assign(K &&key, M &&obj);

	// Inserts the element built from args unless its key is there. A key and
	// a value, or a pair of them, only build the value if the key is new.
	template <class... Args>
	std::pair<iterator, bool> emplace(Args &&...args);

	// Inserts key with the value built from args, unless key is there (then
	// args are left untouched).
	template <class... Args>
	std::pair<iterator, bool> try_emplace(const Key &key, Args &&...args);
	template <class... Args>
	std::pair<iterator, bool> try_emplace(Key &&key, Args &&...args);
	template <class K, class... Args>
		requires detail::flathash::OtherKey<K, Key, Hash, Equal>
	std::pair<iterator, bool> try_emplace(K &&key, Args &&...args);

	// Erases an element, a range or a key. The first two return the iterator
	// after the last erased element; the last returns how many it erased (0
	// or 1).
	iterator erase(const_iterator pos);
	iterator erase(const_iterator first, const_iterator last);
	template <class K = Key>
	size_type erase(const KeyArg<K> &key)
		requires detail::flathash::NotIterator<KeyArg<K>, iterator, const_iterator>;

	void swap(FlatHashMap &other) noexcept(
		std::is_nothrow_swappable_v<Hash> && std::is_nothrow_swappable_v<Equal>);

	// The value of key; at() throws std::out_of_range if it isn't there,
	// operator[] inserts it with a value-initialized value.
	template <class K = Key>
	Value &at(const KeyArg<K> &key);
	template <class K = Key>
	const Value &at(const KeyArg<K> &key) const;
	Value       &operator[](const Key &key);
	Value       &operator[](Key &&key);
	template <class K>
		requires detail::flathash::OtherKey<K, Key, Hash, Equal>
	Value &operator[](K &&key);

	// Lookups. find() returns end() if key isn't there; equal_range() an
	// empty range then, else the range of its element.
	template <class K = Key>
	iterator find(const KeyArg<K> &key);
	template <class K = Key>
	const_iterator find(const KeyArg<K> &key) const;
	template <class K = Key>
	bool contains(const KeyArg<K> &key) const;
	template <class K = Key>
	size_type count(const KeyArg<K> &key) const;
	template <class K = Key>
	std::pair<iterator, iterator> equal_range(const KeyArg<K> &key);
	template <class K = Key>
	std::pair<const_iterator, const_iterator> equal_range(
		const KeyArg<K> &key) const;

	// Hash policy. bucket_count() is capacity(). max_load_factor() is fixed
	// (7/8): max_load_factor(float) does nothing, as in Abseil, for code that
	// calls it. rehash(n) rebuilds the table with at least n slots, or as few as
	// the elements need (rehash(0) shrinks it to fit). reserve(n) makes room
	// for n elements. Both throw std::length_error if n is above max_size().
	float     load_factor() const noexcept;
	float     max_load_factor() const noexcept;
	void      max_load_factor(float ignored) noexcept;
	size_type bucket_count() const noexcept;
	void      rehash(size_type capacity);
	void      reserve(size_type count);

	hasher    hash_function() const;
	key_equal key_eq() const;
};

// From the pairs of a range or of an initializer list, then the capacity,
// Hash and Equal, as in the constructors. Write a range with parentheses:
// FlatHashMap{first, last} is an initializer list of two iterators.
template <std::input_iterator InputIt,
		  class Hash  = DefaultHash<detail::flathash::IterKey<InputIt>>,
		  class Equal = DefaultEqual<detail::flathash::IterKey<InputIt>>>
FlatHashMap(InputIt, InputIt, size_t = 0, Hash = Hash(), Equal = Equal())
	-> FlatHashMap<detail::flathash::IterKey<InputIt>,
				   detail::flathash::IterMapped<InputIt>,
				   Hash,
				   Equal>;

template <class Key, class Value, class Hash = DefaultHash<Key>, class Equal = DefaultEqual<Key>>
FlatHashMap(std::initializer_list<std::pair<Key, Value>>, size_t = 0, Hash = Hash(), Equal = Equal())
	-> FlatHashMap<Key, Value, Hash, Equal>;

// Erases the elements for which pred(element) is true, and returns how many.
template <class Key, class Value, class Hash, class Equal, class Pred>
size_t erase_if(FlatHashMap<Key, Value, Hash, Equal> &map, Pred pred);

// Maps are equal when they have the same keys with equal values.
template <class Key, class Value, class Hash, class Equal>
bool operator==(const FlatHashMap<Key, Value, Hash, Equal> &lhs,
				const FlatHashMap<Key, Value, Hash, Equal> &rhs);

template <class Key, class Value, class Hash, class Equal>
void swap(FlatHashMap<Key, Value, Hash, Equal> &lhs,
		  FlatHashMap<Key, Value, Hash, Equal> &rhs) noexcept(noexcept(lhs.swap(rhs)));

} // namespace TU

// -----------------------------------------------------------------------------
// Benchmarks of uint64_t keys (2026-10-29)
// -----------------------------------------------------------------------------
//
// Three maps from uint64_t to uint64_t: TU::FlatHashMap with DefaultHash,
// absl::flat_hash_map (Abseil 20250127.1, absl::Hash) and std::unordered_map
// (std::hash, libstdc++ 14), with 1K to 10M elements.
// Nanoseconds per operation, the median of 7 runs of each benchmark:
// - find: one find() or count(), for each key of the map in shuffled order
//   (random keys, or the keys 0..n-1), or for as many random keys that
//   aren't there;
// - insert: one try_emplace() or insert() of a new random key, into a map
//   that starts empty and grows (the time includes growing), or that
//   reserve()d room for all of them first;
// - erase or insert (*): erasing the oldest key and inserting a new random
//   one, n times, keeping n elements; one erase or one insert, so half the
//   time of the pair;
// - iterate: visiting one element of a for loop over the whole map.
// "max variation" is the largest coefficient of variation of the 7 runs
// among the containers of the run: the timings of that row are less certain
// when it is large. The rows marked (r) varied by more than 15% and were run
// 3 more times: they show the median of those 3 medians, and the largest
// variation of a run. The insert and erase-or-insert rows of up to 16K keys
// rotate among 4 sets of random keys: the same keys in the same order in every
// iteration let the branch predictor learn the probes.
//
// TU::FlatHashMap / absl::flat_hash_map (above 1 is slower): find of a present
// key 1.06-1.20, of a missing key 0.91-1.21, inserts while growing 0.95-1.38
// and after reserve() 0.80-1.12, erase or insert 0.75-0.95 (faster), iterate
// 0.77-1.79 (slower in small tables: 1.79 at 1K, 0.77-0.96 from 64K up).
// std::unordered_map is slower at everything but finding the keys 0..n-1 (its
// hash is the identity, which suits them) and iterating tables of up to 4K
// elements.
//
// The tables are wider than 80 columns; keep clang-format from rewrapping them.
// clang-format off
//   operation              size  TU::FlatHashMap  absl::flat_hash_map  std::unordered_map  max variation
//   find, key present        1K             2.00                 1.89                2.61             2%
//                            4K             2.14                 1.99                2.64             1%
//                           64K             3.72                 3.40                9.32             4%
//                            1M            17.29                15.70               29.06             2%
//                            4M            23.51                19.63               32.76             6%
//                           10M            29.11                25.49               35.31             2%
//   find, keys 0..n-1        1K             1.90                 1.86                1.59             1%
//                            4K             2.09                 1.96                1.67             2%
//                           64K             3.74                 3.37                3.00             3%
//                            1M            17.29                15.79               17.35             2%
//                            4M            23.42                19.60               21.46             9%
//                           10M            29.03                25.54               21.91             2%
//   find, key missing        1K             1.58                 1.69                3.47             2%
//                            4K             1.69                 1.78                3.25             1%
//                           64K             2.38                 2.20               13.11             1%
//                            1M             3.92                 3.97               30.84             7%
//                            4M             5.94                 6.53               34.34             1%
//                           10M            13.22                10.94               39.31             2%
//   insert, map growing      1K            16.49                11.91               41.39             4%
//                            4K            18.31                14.58               45.37             2%
//                           64K            19.90                16.33               62.31             6%
//                            1M            32.02                29.66              249.20             9%
//                            4M            36.89                38.81              420.56             8%
//                           10M            41.14                43.08              457.48             3%
//   insert, after reserve    1K             4.28                 5.03               35.68             3%
//                            4K             4.38                 5.50               38.59             6%
//                           64K             5.60                 6.60               47.39             2%
//                            1M            19.04                17.86              156.69             4%
//                            4M            27.93                24.90              246.92             6%
//                           10M            36.88                37.41              266.16             2%
//   erase or insert *        1K             5.78                 6.81               22.45             5%
//                            4K             6.36                 8.44               24.17             3%
//                           64K             8.69                10.57               41.26             1%
//                            1M            28.44                31.46              183.30             2%
//                            4M            38.04                40.62              233.68             9%
//                           10M            51.14                54.01              259.91             4%
//   iterate, per element     1K             2.81                 1.57                1.28             3% (r)
//                            4K             2.77                 1.60                2.00             8%
//                           64K             2.80                 3.52                5.68             1%
//                            1M             2.90                 3.73               40.57             1%
//                            4M             2.86                 3.72               58.97             1%
//                           10M             2.83                 2.96               61.76             0%
// clang-format on
// Setup: AMD Ryzen 7 5825U with Radeon Graphics (32 KiB L1D, 512 KiB L2, 16 MiB
// L3), c++ (Debian 14.2.0-19) 14.2.0, Release build with -march=native, one
// process pinned to one physical core (with its SMT sibling and the other
// cores idle) at 4.39-4.53 GHz, the benchmarks in random order (so that clock
// drift affects every container alike), with Google Benchmark. Times depend
// on the machine; the ratios between containers less so.

// -----------------------------------------------------------------------------
// Benchmarks of string keys (2026-10-29)
// -----------------------------------------------------------------------------
//
// Maps from std::string to uint64_t: TU::FlatHashMap with DefaultHash,
// absl::flat_hash_map (Abseil 20250127.1, absl::Hash) and std::unordered_map
// (std::hash, libstdc++ 14), for keys of 12, 40, 100 and 512 characters
// (size/length; 512 only up to 256K keys). Lookups are by std::string_view in
// all three (a transparent hash and std::equal_to<> for std::unordered_map),
// so that none builds a std::string. The operations, "max variation" and the
// rows marked (r) are those of the table above; every row was run 3 more
// times, since the clock of the core dropped during the run.
//
// TU::FlatHashMap / absl::flat_hash_map (above 1 is slower): find of a present
// key 0.75-1.19 (median 0.94), of a missing key 0.79-1.39 (0.96), inserts
// 0.72-1.27 (0.96), erase or insert 0.78-1.31 (0.92), the slowest for keys of
// 512 characters, whose hash reads 16 bytes per step; iterate 1.73-1.82 up
// to 16K keys, faster above. std::unordered_map is slower at everything: up
// to 11 times to find a missing key, about as fast at best to insert.
//
// The hashes alone, nanoseconds per hash, for strings in L1 (throughput:
// independent hashes; latency: each waits for the previous one):
//   length                 1     8    16    32    64   100   256   512  1024
//   throughput  TU        2.02  1.58  1.39  2.55  3.59  5.61 13.53 28.38 66.17
//               Abseil    1.77  1.31  1.30  1.92  4.52  6.65 11.34 19.23 34.74
//               std       2.92  2.92  3.18  4.54  7.76 12.53 25.74 54.48 116.94
//   latency     TU        5.64  5.18  5.08  6.73  9.08 13.06 24.89 46.15 88.71
//               Abseil    5.58  5.08  5.07  5.32 10.51 11.46 15.87 24.04 40.07
//               std       6.50  6.34  7.17  9.06 12.44 17.13 33.92 63.41 122.77
//
// The tables are wider than 80 columns; keep clang-format from rewrapping them.
// clang-format off
//   operation                  size  TU::FlatHashMap  absl::flat_hash_map  std::unordered_map  max variation
//   find, key present         1K/12             6.58                 6.97               13.55             6% (r)
//                             1K/40            10.24                12.32               19.51            19% (r)
//                            1K/100            16.12                19.54               36.23             5% (r)
//                            1K/512            71.25                59.90              107.28             2% (r)
//                            16K/12            12.02                12.18               32.16             4% (r)
//                            16K/40            19.62                21.75               34.91             6% (r)
//                           16K/100            26.22                33.38               61.02             5% (r)
//                           16K/512           205.24               206.72              272.31             4% (r)
//                           256K/12            50.77                52.28              140.33             6% (r)
//                           256K/40           117.23               128.61              179.82             5% (r)
//                          256K/100           142.75               190.02              349.85             4% (r)
//                          256K/512           359.13               354.84              630.50             8% (r)
//                             1M/12            65.18                63.86              199.37             4% (r)
//                             1M/40           127.56               135.04              221.56             5% (r)
//                            1M/100           156.02               200.84              415.10             4% (r)
//   find, key missing         1K/12             3.32                 2.94               11.62            10% (r)
//                             1K/40             5.67                 7.08               15.07             6% (r)
//                            1K/100             9.34                10.23               31.34             4% (r)
//                            1K/512            41.62                29.85               91.63             5% (r)
//                            16K/12             5.17                 4.65               30.10             3% (r)
//                            16K/40             8.86                10.97               34.42             4% (r)
//                           16K/100            13.95                15.02               55.34             6% (r)
//                           16K/512            50.44                40.24              129.44             8% (r)
//                           256K/12            11.03                11.45              129.38            17% (r)
//                           256K/40            39.21                49.54              166.90             7% (r)
//                          256K/100            62.42                71.61              293.52             6% (r)
//                          256K/512           164.59               161.62              451.24             8% (r)
//                             1M/12            28.57                25.07              193.83             4% (r)
//                             1M/40            48.39                59.06              210.46             5% (r)
//                            1M/100            75.60                77.97              341.72             5% (r)
//   insert, map growing       1K/12            25.18                22.27               52.12             8% (r)
//                             1K/40            55.11                56.37               85.98             6% (r)
//                            1K/100            69.32                71.52               96.00             6% (r)
//                            1K/512           164.47               129.67              163.30             6% (r)
//                            16K/12            31.50                32.01               74.38             3% (r)
//                            16K/40            70.47                77.63              108.81             1% (r)
//                           16K/100            87.51                90.91              121.47             2% (r)
//                           16K/512           197.85               173.85              194.84             6% (r)
//                           256K/12            56.81                75.43              165.78             8% (r)
//                           256K/40           226.22               258.02              330.86             7% (r)
//                          256K/100           295.15               320.44              456.75             6% (r)
//                          256K/512           548.50               543.91              600.31             4% (r)
//                             1M/12            78.21               101.70              400.13             5% (r)
//                             1M/40           332.19               373.50              539.69             4% (r)
//                            1M/100           392.14               412.85              658.17             5% (r)
//   insert, after reserve     1K/12             8.89                12.32               44.71            14% (r)
//                             1K/40            35.82                37.04               78.06             7% (r)
//                            1K/100            42.94                45.03               86.76             7% (r)
//                            1K/512            88.28                72.92              144.87             8% (r)
//                            16K/12            15.34                18.17               58.39             5% (r)
//                            16K/40            48.89                50.94               89.71             3% (r)
//                           16K/100            56.35                61.42               99.82             3% (r)
//                           16K/512           115.30               104.54              177.66             6% (r)
//                           256K/12            46.55                55.54              106.62            15% (r)
//                           256K/40           180.34               202.03              213.47            10% (r)
//                          256K/100           229.79               253.46              301.19             6% (r)
//                          256K/512           322.97               320.02              485.70             4% (r)
//                             1M/12            78.96                81.82              254.19             4% (r)
//                             1M/40           271.01               275.97              341.31             5% (r)
//                            1M/100           289.89               308.78              418.39             5% (r)
//   erase or insert *         1K/12             9.84                11.67               39.38             6% (r)
//                             1K/40            26.09                30.06               55.68             6% (r)
//                            1K/100            33.82                36.88               67.09             5% (r)
//                            1K/512            79.39                60.59              131.40             3% (r)
//                            16K/12            15.90                18.31               63.14             2% (r)
//                            16K/40            36.32                40.77               84.40             3% (r)
//                           16K/100            43.95                47.36               95.70             3% (r)
//                           16K/512           109.74                91.31              183.76             6% (r)
//                           256K/12            47.09                60.67              166.68             7% (r)
//                           256K/40           130.05               149.42              293.68             9% (r)
//                          256K/100           167.84               180.14              358.71             7% (r)
//                          256K/512           237.12               222.99              484.81             4% (r)
//                             1M/12            61.25                71.53              343.56             4% (r)
//                             1M/40           186.51               197.65              401.75             6% (r)
//                            1M/100           205.76               213.99              442.08             3% (r)
//   iterate, per element      1K/12             2.80                 1.54                2.96            12% (r)
//                             1K/40             2.81                 1.55                3.49            12% (r)
//                            1K/100             2.81                 1.56                3.11            13% (r)
//                            1K/512             2.80                 1.56                3.80            10% (r)
//                            16K/12             2.83                 1.63                6.56             8% (r)
//                            16K/40             2.83                 1.62                9.12            16% (r)
//                           16K/100             2.83                 1.64               10.39            14% (r)
//                           16K/512             2.83                 1.63               11.35            12% (r)
//                           256K/12             3.56                 3.88               22.52            20% (r)
//                           256K/40             3.55                 3.89               62.84            10% (r)
//                          256K/100             3.68                 3.89               87.54            10% (r)
//                          256K/512             3.68                 3.92               89.89             9% (r)
//                             1M/12             3.47                 3.80               63.86             6% (r)
//                             1M/40             3.48                 3.80               85.80             6% (r)
//                            1M/100             3.32                 3.80               97.38             6% (r)
// clang-format on
// Setup: as for the uint64_t table above, at 4.21-4.54 GHz.

// -----------------------------------------------------------------------------
// Against TU::FlatMap and std::map (2026-11-02)
// -----------------------------------------------------------------------------
//
// TU::FlatMap (a sorted vector), std::map, TU::FlatHashMap and
// std::unordered_map from uint64_t to uint64_t, random keys. Nanoseconds per
// operation, the median of 9 runs of the four containers in a rotating order;
// insert one by one starts from an empty container, build from a range uses the
// range constructor. Measured while the machine was busy (load average about
// 129 on 16 hardware threads, every CPU busy), pinned to one CPU and timed in
// the CPU time of the thread, so that waiting to be scheduled isn't counted;
// the times are about twice those of an idle machine (the other hardware thread
// of the core and the caches are shared), but a second run gave the same times
// within 15%, and the ratios hold.
//
// TU::FlatHashMap is faster than std::map at everything: 6-21 times to find a
// present key, 9-73 times a missing one, 2.4-4.2 times to insert or build, and
// 1.2-29 times to iterate. Against TU::FlatMap (above 1: TU::FlatHashMap is
// faster): find of a present key 4.6-7.1, of a missing key 7-25, insert one by
// one 3.1 at 1K and 108 at 64K (each insertion shifts half of the vector, so
// it wasn't run at 1M), build from a range 0.43-1.0 and iterate 0.15-0.17:
// TU::FlatMap is the better choice for a map built once and then mostly
// iterated.
//
// clang-format off
//   operation              size      TU::FlatMap         std::map  TU::FlatHashMap    unordered_map
//   find, key present         1K            25.33            33.59             5.30             6.89
//                            64K           119.88           212.46            26.03            24.64
//                             1M           247.03           720.40            34.64            53.98
//   find, key missing         1K            29.24            35.72             4.11             9.54
//                            64K           119.92           177.85             7.06            22.77
//                             1M           251.98           743.76            10.19            58.55
//   iterate, per element      1K             0.77             5.66             4.60             2.62
//                            64K             0.61            22.60             4.14             9.00
//                             1M             0.65           115.88             4.06            49.02
//   build from a range        1K            33.43           109.00            45.66            67.32
//                            64K            59.95           222.77            58.38           112.70
//                             1M            76.58           720.11           177.35           353.55
//   insert one by one         1K           146.59           117.88            47.21            65.88
//                            64K          6722.38           260.45            62.19           112.77
//                             1M              n/a           699.29           179.16           356.71
// clang-format on
// Setup: AMD Ryzen 7 5825U, g++ 14.2 -O3 -march=native, pinned to one CPU.
