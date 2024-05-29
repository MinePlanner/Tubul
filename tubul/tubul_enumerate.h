//
// Created by Carlos Acosta on 19-10-23.
//


#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

namespace TU {

    /** This is basically the same std concept to check that something is an actual
     * range that can be iterated with begin() and end().
     */
    template<typename C>
    concept TubulIterable = std::ranges::range<C>;
    /** This templated function receives an standard iterable container (via begin/end)
     * and returns a simple wrapper that will return tuples (i,item), where i is the index
     * of item inside the container. This follows the same idea as Python's enumerate.
     * The template type T is the container to be iterated.
     */
    template <TubulIterable T>
    constexpr auto enumerate(T && iterable)
    {
        using TIter = decltype(std::begin(std::declval<T>()));

        struct iterator
        {
            size_t i;
            TIter iter;
            bool operator != (const iterator & other) const { return iter != other.iter; }
            void operator ++ () { ++i; ++iter; }
            auto operator * () const { return std::tie(i, *iter); }
        };
        struct iterable_wrapper
        {
            T iterable;
            auto begin() { return iterator{ 0, std::begin(iterable) }; }
            auto end() { return iterator{ 0, std::end(iterable) }; }
        };
        return iterable_wrapper{ std::forward<T>(iterable) };
    }


}// end namespace TU