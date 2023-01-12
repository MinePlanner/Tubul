//
// Created by Carlos Acosta on 12-01-23.
//

#pragma once
#include <string>
#include <algorithm>

namespace TU {


template <typename IteratorType>
std::string join(IteratorType begin, IteratorType end, std::string const& joiner)
{
	std::string result;
	//if we are given nothing to join, that's it.
	if (begin == end)
		return result;
	//Check the total size of the input strings.
	size_t total_size = 0;
	size_t total_items = std::distance(begin, end);
	std::for_each(begin,end,[&]( std::string const& it){ total_size+= it.size();});
	//If the sum is 0, means all strings are empty. Should we really do something?
	//I think it's debatable, but for example for CSV files, we would still want
	// the comma separated empty strings, so i prefer to continue, although some
	// cases are questionable.

	//If we only have one item, there's nothing more to do.
	if (total_items == 1)
	{
		result = *begin;
		return result;
	}
	//If the joiner is empty, we just concatenate the strings on the result
	if (joiner.empty() )
	{
		result.reserve(total_size );
		std::for_each(begin,end,[&]( std::string const& it){ result.append(it);});
		return result;
	}
	//Note that we already stablished there's at least 2 elements!
	result.reserve(total_size + total_items*joiner.size() );
	result.append(*begin);
	++begin;
	std::for_each(begin,end,[&]( std::string const& it){ result.append(joiner); result.append(it); });

	return result;
}

template <typename ContainerType>
std::string join(ContainerType const& container, std::string const& joiner)
{
	return join(std::begin(container), std::end(container), joiner );
}
}
