//
// Created by Carlos Acosta on 19-10-23.
//


#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <tuple>

namespace TU {

    /** This templated function receives an standard iterable container (via begin/end)
     * and returns a simple wrapper that will return tuples (i,item), where i is the index
     * of item inside the container. This follows the same idea as Python's enumerate.
     * The template type T is the container to be iterated. The second and third are in big
     * part to ensure that T actually contains a begin/end function.
     */
    template <typename T,
            typename TIter = decltype(std::begin(std::declval<T>())),
            typename = decltype(std::end(std::declval<T>()))>
    constexpr auto enumerate(T && iterable)
    {
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