#pragma once

// tubul_flat_hash.h - TU::DefaultHash and TU::DefaultEqual, the default hash
// and equality of the keys of TU::FlatHashMap and TU::FlatHashSet. They can
// also be used on their own, for other containers.
//
// DefaultHash<Key> is the first of these that applies to Key, in this order:
// - integers, enums and pointers: their value, mixed. A pointer is hashed and
//   compared by its address, so a const char* key is its address, not its
//   text, and a container keyed by pointers iterates in a different order in
//   every run (addresses change between runs);
// - floating point values: their bits, mixed, with -0.0 as 0.0 (they compare
//   equal);
// - structs that declare hashFields(key): the fields it returns. The struct
//   lists the members that make up its identity with a function found by
//   argument-dependent lookup (a hidden friend, as below), returning them as
//   a tuple:
//     struct CategoryKey {
//         uint64_t m_category;
//         int32_t  m_time;
//         uint16_t m_realization; // and 2 bytes of padding
//         bool operator==(const CategoryKey&) const = default;
//         friend auto hashFields(const CategoryKey& key) {
//             return std::tie(key.m_category, key.m_time, key.m_realization);
//         }
//     };
//   hashFields() must return exactly the members that operator== compares:
//   one more makes equal keys hash differently, which breaks lookups. It is
//   needed for structs with padding (whose bytes are garbage), with floating
//   point members, or whose operator== ignores some members;
// - std::pair and std::tuple: their elements, as if hashFields() returned
//   them. The order of the elements counts: (a, b) and (b, a) differ;
// - keys without padding (std::has_unique_object_representations_v), such as
//   structs of integers, enums and pointers, or arrays of integers: their
//   bytes, with no code at all. An operator== that ignores some members
//   needs hashFields() instead, or equal keys would hash differently;
// - std::array: its elements; std::vector: its elements and its length. The
//   elements may be of any type that DefaultHash hashes (numbers, strings,
//   pairs, nested vectors, structs with hashFields()...), and must have an
//   operator==, since arrays and vectors are compared with theirs;
// - other keys with a std::hash: its result, mixed.
// A key with none of these doesn't compile, with an error that shows how to
// declare hashFields(). Each field or element is one 64-bit word: integers,
// enums and pointers as they are, floating point values by their bits (with
// -0.0 as 0.0, since they compare equal), anything else (a string, a nested
// pair, a struct with hashFields()) by its own DefaultHash.
//
// Floating point keys are compared with operator==, as in std::unordered_map
// and absl::flat_hash_map, so a NaN equals nothing, not even itself: every
// insertion of a NaN key (or of a key that contains one) adds an element,
// which lookups never find; only iterators, erase_if() and clear() remove it.
// To use NaN as a key, map it to one chosen value first, or pass an Equal
// that treats NaNs as equal (and a Hash that gives them one hash).
//
// std::string and std::string_view hash their characters, the same for both,
// and are transparent: a container of either finds keys given as either, or
// as a string literal, without building a std::string.
//
// The results are well mixed (every bit depends on every bit of the key),
// which is_avalanching declares, so the containers use them as they are. They
// are stable, the same on every run and on every 64-bit platform of the same
// byte order, with two exceptions: pointers (and keys that contain them)
// change in every run with the addresses, and keys hashed by their std::hash
// depend on the standard library. They are not meant for keys chosen by an
// attacker.
//
// Benchmarks of compound keys (pairs, tuples, arrays, vectors) against
// absl::flat_hash_set and std::unordered_set: see the end of this file.

#include <cstddef>
#include <string>
#include <string_view>

namespace TU
{

// The default hash (see the top of this file).
template <class Key>
struct DefaultHash
{
	using is_avalanching = void;
	size_t operator()(const Key &key) const;
};

// The default equality: element by element (each with the DefaultEqual of
// its type) for std::pair and std::tuple, else operator== if Key has one,
// else the fields of hashFields(), else the bytes (keys without padding).
template <class Key>
struct DefaultEqual
{
	bool operator()(const Key &lhs, const Key &rhs) const;
};

// Strings: their characters, transparently (see the top of this file).
template <>
struct DefaultHash<std::string>
{
	using is_avalanching = void;
	using is_transparent = void;
	size_t operator()(std::string_view text) const;
};
template <>
struct DefaultHash<std::string_view> : DefaultHash<std::string>
{
};

template <>
struct DefaultEqual<std::string>
{
	using is_transparent = void;
	bool operator()(std::string_view lhs, std::string_view rhs) const;
};
template <>
struct DefaultEqual<std::string_view> : DefaultEqual<std::string>
{
};

} // namespace TU

// -----------------------------------------------------------------------------
// Benchmarks of compound keys (2026-10-30)
// -----------------------------------------------------------------------------
//
// TU::FlatHashSet with DefaultHash, absl::flat_hash_set (Abseil 20250127.1,
// absl::Hash) and std::unordered_set (also with absl::Hash, since the standard
// library has no hash for these keys), for keys that DefaultHash hashes
// element by element; the vectors have 8 integers or 4 doubles. The
// operations, "max variation" and the rows marked (r) are those of
// tubul_flat_hash_set.h; here the rows that varied by more than 15% were run
// 3 more times.
//
// TU::FlatHashSet / absl::flat_hash_set (above 1 is slower), the median over
// the six keys: inserts while growing 1.02 (0.93-1.26), find of a present key
// 1.14 (0.80-1.35), of a missing key 1.14 (0.84-1.54), erase or insert 0.90
// (0.82-1.03), iterate 0.80 (0.69-1.88).
//
// The tables are wider than 80 columns; keep clang-format from rewrapping them.
// clang-format off
//   std::pair<uint32_t, uint32_t>
//   operation            size  TU::FlatHashSet  absl::flat_hash_set  std::unordered_set  max variation
//   insert, set growing    1K            16.39                13.10               43.65            14%
//                         64K            20.58                17.71               68.10             2%
//                          1M            26.64                26.11              268.81             4%
//                          4M            33.70                35.29              430.99            34% (r)
//   find, key present      1K             2.70                 2.37                2.98            52% (r)
//                         64K             4.59                 3.68               11.06             5%
//                          1M            18.10                14.81               40.64            12%
//                          4M            29.00                21.41               47.05             5%
//   find, key missing      1K             2.14                 1.78                3.57             5%
//                         64K             2.92                 2.42               15.12             3%
//                          1M             4.76                 4.26               41.18            76% (r)
//                          4M             7.13                 7.16               47.44            97% (r)
//   erase or insert *      1K             6.31                 7.71               25.78             7%
//                         64K             9.17                11.25               44.63           133% (r)
//                          1M            23.59                26.23              194.17             6%
//                          4M            40.24                44.84              248.13            73% (r)
//   iterate, per key       1K             2.77                 1.47                1.56             9% (r)
//                         64K             2.80                 3.47                5.98            16% (r)
//                          1M             2.81                 3.62               41.75            37% (r)
//                          4M             2.80                 3.62               59.28             1%
//
//   std::tuple<uint64_t, uint32_t, uint16_t>
//   operation            size  TU::FlatHashSet  absl::flat_hash_set  std::unordered_set  max variation
//   insert, set growing    1K            18.64                15.63               46.09            15%
//                         64K            22.64                21.05               74.07             7%
//                          1M            36.48                37.69              319.62            45% (r)
//                          4M            42.43                44.15              469.04            51% (r)
//   find, key present      1K             3.48                 3.19                3.74             1%
//                         64K             6.01                 4.97               12.59            99% (r)
//                          1M            24.79                21.08               48.83             3%
//                          4M            34.76                26.59               54.87            12%
//   find, key missing      1K             2.69                 2.19                3.61             3% (r)
//                         64K             3.57                 2.87               16.27             2%
//                          1M             6.65                 6.86               50.61            51% (r)
//                          4M             8.49                 8.28               55.40            83% (r)
//   erase or insert *      1K             7.14                 8.54               27.67             2%
//                         64K            10.48                12.71               47.93             3%
//                          1M            31.74                35.03              220.33            58% (r)
//                          4M            42.40                45.61              259.12             8%
//   iterate, per key       1K             2.77                 1.58                1.95            10%
//                         64K             2.77                 3.51                6.68             4%
//                          1M             2.88                 3.73               50.80             4%
//                          4M             2.91                 3.71               63.36             4%
//
//   std::array<uint32_t, 4>
//   operation            size  TU::FlatHashSet  absl::flat_hash_set  std::unordered_set  max variation
//   insert, set growing    1K            17.05                13.57               44.75             6%
//                         64K            21.57                17.88               72.06            52% (r)
//                          1M            33.23                32.39              316.57             7%
//                          4M            42.74                41.52              464.39            55% (r)
//   find, key present      1K             3.21                 2.42                2.91             2%
//                         64K             5.30                 4.30               10.69           105% (r)
//                          1M            22.92                20.12               40.30             4%
//                          4M            31.97                24.68               43.84             5%
//   find, key missing      1K             2.66                 1.73                3.47             7%
//                         64K             3.42                 2.51               14.29             2%
//                          1M             6.17                 6.10               41.52             7%
//                          4M             8.15                 7.69               45.04             9%
//   erase or insert *      1K             6.08                 7.35               24.87            35% (r)
//                         64K            10.11                12.12               45.57             4%
//                          1M            31.21                34.58              218.75            96% (r)
//                          4M            44.65                43.39              254.02            72% (r)
//   iterate, per key       1K             2.84                 1.54                2.28            11%
//                         64K             2.81                 3.50                6.57             7%
//                          1M             2.95                 3.70               51.79             3%
//                          4M             2.95                 3.72               64.20             4%
//
//   std::array<double, 3>
//   operation            size  TU::FlatHashSet  absl::flat_hash_set  std::unordered_set  max variation
//   insert, set growing    1K            20.87                17.67               47.83             5%
//                         64K            25.85                24.04               75.07            50% (r)
//                          1M            41.23                43.30              321.92             7%
//                          4M            51.02                54.30              470.19            56% (r)
//   find, key present      1K             5.61                 4.91                5.48             1%
//                         64K             8.60                 7.51               16.87             5%
//                          1M            37.02                31.96               73.24             2%
//                          4M            49.13                40.31               81.15             6%
//   find, key missing      1K             3.94                 3.26                4.36             2%
//                         64K             5.06                 4.06               19.25             8% (r)
//                          1M             8.86                 8.66               66.74            39% (r)
//                          4M            10.50                10.30               73.77            13%
//   erase or insert *      1K             8.90                10.04               29.01             4%
//                         64K            12.85                14.64               48.66            86% (r)
//                          1M            35.72                43.68              216.12            73% (r)
//                          4M            45.88                55.70              257.50            23% (r)
//   iterate, per key       1K             3.00                 1.72                2.32             8%
//                         64K             2.98                 4.18                6.81             3%
//                          1M             3.07                 4.47               52.95            13%
//                          4M             3.13                 4.48               63.66             5%
//
//   std::vector<uint32_t> of 8
//   operation            size  TU::FlatHashSet  absl::flat_hash_set  std::unordered_set  max variation
//   insert, set growing    1K            56.14                52.23               79.37             2%
//                         64K            71.34                70.53              111.76           109% (r)
//                          1M           275.09               271.89              446.59            25% (r)
//                          4M           330.78               337.31              569.67            43% (r)
//   find, key present      1K             7.85                 8.59               10.18            10%
//                         64K            17.29                21.12               28.44            98% (r)
//                          1M            91.55               113.21              158.90             3%
//                          4M           107.63               134.10              168.56             4%
//   find, key missing      1K             5.11                 3.80                6.90             6%
//                         64K             6.58                 5.19               23.42             3%
//                          1M            14.22                15.92              105.10            83% (r)
//                          4M            17.19                19.10              112.87             8%
//   erase or insert *      1K            22.15                22.56               41.47            11%
//                         64K            45.10                48.76               79.12           109% (r)
//                          1M           139.85               153.24              355.00             4%
//                          4M           167.06               185.36              423.90            42% (r)
//   iterate, per key       1K             2.81                 1.67                2.87            15%
//                         64K             3.09                 3.59                8.07            13%
//                          1M            11.56                 7.42               67.45            11%
//                          4M            12.20                 8.03               72.97            10%
//
//   std::vector<double> of 4
//   operation            size  TU::FlatHashSet  absl::flat_hash_set  std::unordered_set  max variation
//   insert, set growing    1K            58.34                58.56               79.92             3%
//                         64K            74.08                76.86              112.56             4%
//                          1M           277.19               282.52              461.53            32% (r)
//                          4M           340.24               365.72              567.95             5%
//   find, key present      1K            11.45                10.95               14.19             9%
//                         64K            23.30                24.35               32.05            83% (r)
//                          1M           127.50               124.23              165.74             3%
//                          4M           152.26               147.69              172.88             3%
//   find, key missing      1K             6.45                 5.37                7.32             2%
//                         64K             7.54                 6.43               24.77             6%
//                          1M            15.19                17.99              103.16            90% (r)
//                          4M            19.78                22.74              110.68            10%
//   erase or insert *      1K            26.24                26.12               45.20             3%
//                         64K            49.21                49.49               84.88            34% (r)
//                          1M           142.34               147.89              349.79             6%
//                          4M           179.82               186.58              397.36             4%
//   iterate, per key       1K             2.91                 1.73                2.88             5%
//                         64K             3.06                 4.17                8.05             8%
//                          1M            11.19                 9.38               66.70            19% (r)
//                          4M            11.91                 9.67               72.92             2%
// clang-format on
// Setup: as for the uint64_t table of tubul_flat_hash_map.h, at 4.31-4.54 GHz.
