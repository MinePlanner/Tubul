//
// Created by Carlos Acosta on 04-05-23.
//

#pragma once
#include <string_view>
#include <cstdlib>
#include <tuple>

namespace TU{
/** Set of simple functions to perform tasks that are common
 * but regularly require typing a lot more than just these names
 * in order to do them. For example the strToXXX use the from_chars functions
 * which are awesome, but require a couple extra steps to be used which are
 * very cumbersome.
 */
    bool isRegularFile(const std::string_view& name);
    size_t countCharInFile( const std::string_view& filename, char c);
    double strToDouble(const std::string_view& p);
    int strToInt(const std::string_view& p);


//Simple wrapper for common task when using str_views to convert to types.
    inline
    constexpr auto strview_range(std::string_view s) noexcept {
        return std::make_tuple(s.data(), s.data() + s.size());
    }
}
