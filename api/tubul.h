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

	// strings
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

	struct Argument;

	void parseArgsOrDie(int argc, char** argv);
	Argument addArgument(std::string const& short_form, std::string const& long_form);

	template<typename T>
	T getArg(std::string const& param);

	template <typename T>
	std::optional<T> isArgPresent( std::string const& param);
}