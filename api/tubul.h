//
// Created by Nicolas Loira on 11-01-23.
//

#pragma once
#include <vector>
#include <string>
#include <string_view>

namespace TU{
    void init();
    int getVersion();


	std::vector< std::string_view > split(std::string const& input);
	std::vector< std::string_view > split(std::string const& input, std::string const& delims);

	template <typename ContainerType>
	std::string join(ContainerType const& container, std::string const& joiner);

	template <typename IteratorType>
	std::string join(IteratorType begin, IteratorType end, std::string const& joiner);
}