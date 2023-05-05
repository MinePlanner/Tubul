//
// Created by Carlos Acosta on 16-01-23.
//

#include "tubul_irange.h"

namespace TU{

tubul_range irange(std::size_t begin, std::size_t end)
{
	return {begin, end};
}

tubul_range irange(std::size_t end)
{
	return {0, end};
}

tubul_skip_range irange(std::size_t begin, std::size_t end, std::size_t step)
{
	return {begin, end, step};
}
}