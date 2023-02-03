//
// Created by Nicolas Loira on 16-03-23.
//


#pragma once
#include <vector>
#include <string_view>

////////////
// Strings
///////////

namespace TU {
/** String handling functions.
 * Split will return a vector of string_views with the tokens detected. Do note
 * there are 2 versions. If you don't pass the argument, the delimiter is assumed to be
 * a space and also consecutive spaces are considered one. When the delimiter is given
 * the consecutive delimiters are considered as if they are surrounding an empty string. This
 * can be very important when working with csv files.
 *
 * Join is the opposite to split, where you can pass the delimiters and a set of strings
 * to be merged, and returns the new resulting string. Do note you can pass iterators if
 * you don't want to merge a whole container.
 */
    std::vector<std::string_view> split(std::string const &input);

    std::vector<std::string_view> split(std::string const &input, std::string const &delims);

    std::vector<std::string_view> split(std::string_view const &input);

    std::vector<std::string_view> split(std::string_view const &input, std::string const &delims);

    template<typename ContainerType>
    std::string join(ContainerType const &container, std::string const &joiner);

    template<typename IteratorType>
    std::string join(IteratorType begin, IteratorType end, std::string const &joiner);
}