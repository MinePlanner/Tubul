//
// Created by Carlos Acosta on 07-02-23.
//

#pragma once
#include <tuple>

namespace TU
{
enum class DataType
{
	INTEGER,
	DOUBLE,
	STRING
};

enum class ColumnHeaders
{
	YES,
	NO
};
enum class RowHeaders
{
	YES,
	NO
};

}

namespace TU {
    template <typename TT>
    struct hash
    {
        size_t
        operator()(TT const& tt) const
        {
            return std::hash<TT>()(tt);
        }
    };

    namespace {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780
        template<class T>
        inline void hash_combine(size_t &seed, T const &v) {
            seed ^= TU::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        // Recursive template code derived from Matthieu M.
        template<class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashTupleValueImpl {
            static void apply(size_t &seed, Tuple const &tuple) {
                HashTupleValueImpl<Tuple, Index - 1>::apply(seed, tuple);
                hash_combine(seed, std::get<Index>(tuple));
            }
        };

        template<class Tuple>
        struct HashTupleValueImpl<Tuple, 0> {
            static void apply(size_t &seed, Tuple const &tuple) {
                hash_combine(seed, std::get<0>(tuple));
            }
        };
    }

    template<typename ... TT>
    struct hash<std::tuple<TT...>> {
        size_t
        operator()(std::tuple<TT...> const &tt) const {
            size_t seed = 0;
            HashTupleValueImpl<std::tuple<TT...> >::apply(seed, tt);
            return seed;
        }
    };

}