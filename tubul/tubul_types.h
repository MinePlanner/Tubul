//
// Created by Carlos Acosta on 07-02-23.
//

#pragma once
#include <tuple>
#include <cstddef>
#include <functional>

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

/** Functions to work with Enumeration types, to cast them to and from
 * the underlying type used to represent them. Useful for printing or
 * using the enumeration as indices of vectors for example.
 */
template<typename EnumType>
auto toNumber(EnumType val) -> std::underlying_type_t<EnumType> {
    return static_cast< std::underlying_type_t<EnumType> > (val);
}

template<typename EnumType>
auto toEnum( std::underlying_type_t<EnumType> val) -> EnumType {
    return static_cast< EnumType >(val);
}

/** This is so any value for which we don't provide an especialization for
 * will use std::hash as expected. Tuple objects should match the template below!.
 */
template <typename TT>
struct hash
{
    size_t
    operator()(TT const& tt) const
    {
        return std::hash<TT>()(tt);
    }
};

namespace HashImpl {

    // Code from boost
    // Reciprocal of the golden ratio helps spread entropy
    //     and handles duplicates.
    // See Mike Seymour in magic-numbers-in-boosthash-combine:
    //     http://stackoverflow.com/questions/4948780
    template<class T>
    void hash_combine(size_t &seed, T const &v) {
        seed ^= TU::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    /** Variadic function to calculate the hash of an arbitrary number of arguments
     * This function will call the combiner over every parameter in the
     * pack args. The combiner function receives seed by reference and each time it is called
     * will call TU::hash to obtain the hash of the given parameter and combine
     * it with the current value of seed.
     */
    template< typename... Args>
    auto hashValueCalculator(const Args&... args ) -> size_t{
        size_t seed = 0;
        (hash_combine(seed, args), ...);
        return seed;
    }

    /** Variadic function needed to "catch" the type of the tuple and the
     * parameter pack that we need to correctly instantiate hashValueCalculator. This
     * is needed because the call of hashValueCalculator over the tuple values happens
     * way inside std::apply/std::invokes, and when we pass hashValueCalculator without
     * the template, std::apply will not know how solve the instantiation.
     * Do note that we can merge hashValueCalculator inside this function by using a variadic lambda
     * and it should work, but i don't see much point in doing that (besides proving we can)
     */
    template< typename... TupleArgs>
    auto hashTupleImpl(const std::tuple<TupleArgs...>& tt) -> size_t {
        return std::apply(hashValueCalculator<TupleArgs...>,tt);
    }

}

/** Specific implementation for tuples.
 *
 */

template<typename ... TT>
struct hash<std::tuple<TT...>> {
    size_t
    operator()(std::tuple<TT...> const &tt) const {
        return HashImpl::hashTupleImpl(tt);
    }
};

}