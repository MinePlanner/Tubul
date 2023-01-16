//
// Created by Nicolas Loira on 11-01-23.
//

#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <tubul_defs.h>

namespace TU{
    void init();
    int getVersion();

	tubul_range irange(size_t end);
	tubul_range irange(size_t begin, size_t end);
	tubul_skip_range irange(size_t begin, size_t end, size_t step);

	std::vector< std::string_view > split(std::string const& input);
	std::vector< std::string_view > split(std::string const& input, std::string const& delims);

	std::vector< std::string_view > split(std::string_view const& input);
	std::vector< std::string_view > split(std::string_view const& input, std::string const& delims);

	template <typename ContainerType>
	std::string join(ContainerType const& container, std::string const& joiner);

	template <typename IteratorType>
	std::string join(IteratorType begin, IteratorType end, std::string const& joiner);




}