#pragma once

// tubul_flat_hash_map_type.h - the types that tubul_flat_hash_map.h needs to
// declare its interface (namespace TU::detail::flathash). Not meant to be
// included on its own.

#include <iterator>
#include <type_traits>

#include "tubul_flat_hash_table_type.h"

namespace TU::detail::flathash
{

// The key and value types of a range of pairs (deduction guides).
template <std::input_iterator InputIt>
using IterKey =
	std::remove_const_t<typename std::iter_value_t<InputIt>::first_type>;
template <std::input_iterator InputIt>
using IterMapped = std::iter_value_t<InputIt>::second_type;

} // namespace TU::detail::flathash
