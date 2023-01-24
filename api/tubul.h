//
// Created by Nicolas Loira on 11-01-23.
//

#pragma once

#include <vector>
#include <stdexcept>
#ifndef TUBUL_MACOS
#include <source_location>
#endif
#include <string>
#include <string_view>

#include <optional>
#include <tubul_defs.h>

namespace TU {

    // setup
    void init();

    int getVersion();

	/** Simple range iterators.
	 * The idea is that you can use the irange functions to iterate over a range
	 * containing (0, N-1) just the same way you would write a for loop. For example
	 * a vector V with N elements can be iterated by
	 * 					for (auto i: TU::irange(V.size())
	 * The irange with just 1 param, assumes you start from 0, it's the same as using
	 * TU::irange(0, V.size(), but there are too many cases for when you want to iterate
	 * a slice of things in a container.
	 * The irange option with the "step" parameter is equivalent to iterate doing it+=step
	 * on each iteration, which can be useful for example to go over the odd/even numbers
	 * in a range, looping over multiples or what not. Be careful that the begin-end range
	 * does NOT need to be an exact multiple of step for ease of use (like iterate multiples
	 * of 3 between 0 and 10 would be TU::irange(0,11,3)), but that could prove a bit confusing
	 * for some particular use cases.
	 */
	tubul_range irange(size_t end);
	tubul_range irange(size_t begin, size_t end);
	tubul_skip_range irange(size_t begin, size_t end, size_t step);

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
	std::vector< std::string_view > split(std::string const& input);
	std::vector< std::string_view > split(std::string const& input, std::string const& delims);
    std::vector<std::string_view> split(std::string_view const &input);
    std::vector<std::string_view> split(std::string_view const &input, std::string const &delims);

    template<typename ContainerType>
    std::string join(ContainerType const &container, std::string const &joiner);

    template<typename IteratorType>
    std::string join(IteratorType begin, IteratorType end, std::string const &joiner);

	// logger
#ifdef TUBUL_MACOS
	[[nodiscard]] std::runtime_error throwError(const std::string &msg, int line = __builtin_LINE(),
												const char *file = __builtin_FILE(),
												const char *function = __builtin_FUNCTION());
#else
	[[nodiscard]] std::runtime_error throwError(const std::string &message,
												const std::source_location location =
													std::source_location::current());
#endif

	/** Argument parsing facilities.
	 * You can parse arguments command line arguments with the functions on this section.
	 * For adding arguments, you can use addArgument to set up a given argument
	 * Example:
	 * 		TU::addArgument("-f", "--fruit")
	 * 				.defaultValue("apple");
	 * 		TU::addArgument("-a", "--allowance")
	 * 				.setAsDouble()
	 * 				.required();
	 * 		TU::addArgument("-i", "--invitees")
	 * 				.setAsList();
	 * 		TU::addArgument("-v", "--verbose)
	 * 				.flag();
	 *
	 * 	With parseArgsOrDie you call the argument handler to do its magic.
	 * 	Then you can use getArg to retrieve the value of a given argument (be consistent with
	 * 	expected values and default types!) or query the existence of a given argument with isArgPresent
	 * 	which returns an optional of the adequate type.
	 * 	Be mindful that if you expect something as integer or double, you have to set it
	 * 	explicitly while configuring the argument. You can also set something to expect
	 * 	a list of strings with SetAsList (it will expect at least one value!), in which case the
	 * 	expected return is a vector of strings.
	 */


	struct Argument;

	void parseArgsOrDie(int argc, char** argv);
	Argument addArgument(std::string const& short_form, std::string const& long_form);

	template<typename T>
	T getArg(std::string const& param);

	template <typename T>
	std::optional<T> isArgPresent( std::string const& param);
}