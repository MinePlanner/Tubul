//
// Created by Carlos Acosta on 19-10-23.
//


#pragma once
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
            iterator operator ++ () {
                ++i;
                ++iter;
                return *this;
            }
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



    template <typename ... Args, std::size_t ... Index>
    auto any_match_impl(std::tuple<Args...> const &lhs,
                        std::tuple<Args...> const &rhs,
                        std::index_sequence<Index...>) -> bool {
        auto result = false;
        result = (... | (std::get<Index>(lhs) == std::get<Index>(rhs)));
        return result;
    }

    template <typename ... Args>
    auto any_match(std::tuple<Args...> const & lhs, std::tuple<Args...> const & rhs)
        -> bool
    {
        return any_match_impl(lhs, rhs, std::index_sequence_for<Args...>{});
    }


    //This heavily influenced by the zip view implementantion at https://github.com/alemuntoni/zip-views
    //It follows the same idea as enumerate, but the tuple handling makes it a looot more
    //confusing due the heavy use of ellipsis to work over all the ranges provided.
    template <TubulIterable... T>
    constexpr auto zip(T&& ...  iterables) {

        struct zip_iterator {
            using value_type = std::tuple<std::ranges::range_reference_t<T>...>;
            std::tuple<std::ranges::iterator_t<T>...> iter_tuple;

            auto operator++() -> zip_iterator&
            {
                std::apply([](auto && ... args){ ((++args), ...); }, iter_tuple);
                return *this;
            }

            auto operator++(int) -> zip_iterator
            {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            auto operator!=(zip_iterator const & other) const
            {
                return not any_match(iter_tuple, other.iter_tuple);
            }

            auto operator*() -> value_type
            {
                return std::apply([](auto && ... args){
                        return value_type(*args...);
                    }, iter_tuple);
            }

        };
        struct zipper_wrapper {
            std::tuple<T...> iterables;

            auto begin() {
                using std::ranges::begin;
                return std::apply([](auto &&... args) {
                    return zip_iterator{std::make_tuple(begin(args)...)};
                }, iterables);
            }

            auto end()
            {
                using std::ranges::end;
                return std::apply([](auto && ... args){
                    return zip_iterator{std::make_tuple(end(args)...)};
                    }, iterables);
            }
        };

        return zipper_wrapper{std::tie( std::forward<T>(iterables)...) };
    }


}// end namespace TU