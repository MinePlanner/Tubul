//
// Created by Carlos Acosta on 16-01-23.
//

#include "tubul_irange.h"

namespace TU{

tubul_range irange(size_t begin, size_t end)
{
	return tubul_range(begin, end);
}

tubul_range irange(size_t end)
{
	return tubul_range(0, end);
}

tubul_skip_range irange(size_t begin, size_t end, size_t step)
{
	return tubul_skip_range(begin, end, step);
}
}