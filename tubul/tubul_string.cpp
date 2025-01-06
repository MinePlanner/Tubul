//
// Created by Carlos Acosta on 12-01-23.
//

#include <vector>
#include <deque>
#include <list>
#include <set>
#include <string>
#include <string_view>
#include <algorithm>
#include <type_traits>
#include "tubul_string.h"

namespace TU
{
namespace detail {

	template <typename T>
	concept StringLikeType =  std::is_same<std::string,T>::value || std::is_same<std::string_view,T>::value;
/***
 * Split function for merging delimiters. This version is useful for separating
 * words in phrases where there could be multiple spaces by mistake or any reason.
 * @param input string to split.
 * @param delims string containing characters to use as delimiters
 * @return vector with string views of the different found tokens.
 */
template < StringLikeType StringType>
std::vector< std::string_view > split_merge(StringType const& input, std::string const& delims)
{
	//The vector to contain the results
	std::vector< std::string_view > results;
	if (input.empty())
		return results;
	//check the "real" begin and end in case there are delimiters at
	//the beginning or end of input.
	//check if input only contains delimiters
	size_t start = input.find_first_not_of(delims);
	size_t end = std::string::npos;
	if ( start == end )
			return results;
	//Getting the pointer to the first character of the string (note
	//it may be a delimiter).
	const char* ptr = input.data();
	//we already know there's at least one character that is not a delimiter
	//but may be the only one.
		end = input.find_last_not_of(delims) + 1;
	if ( end == start )
	{
		results.emplace_back(ptr+start, 1);
		return results;
	}

	//We can now loop de string starting from the real start: first
	//character that is not a delimiter, and create a string view for
	//the range
	auto found = input.find_first_of(delims, start);
	while (found != std::string::npos && found < end)
	{
		results.emplace_back(ptr+start, found-start);
		//Find the next non delimiter character
			start = input.find_first_not_of(delims, found+1);
		//starting from the next non-delimiter, try to find the next delimiter.
		found = input.find_first_of(delims, start);
	}

	//Add the last token assuming we have something between the last found delimiter
	//the end of the string.
	if (start < end)
	{
		std::string_view last_token(ptr+start, end-start);
		results.push_back( last_token );
	}
	return results;

}


/***
 * Split function to strictly separate delimiters, even if that results in empty strings.
 * This is very useful when handling things like lines coming from a csv file that
 * are structured in a certain way and consecutive delimiters may mean an actual
 * empty entry in a row.
 * @param input string to split.
 * @param delims string containing characters to use as delimiters
 * @return vector with string views of the different found tokens.
 */
template < StringLikeType StringType>
std::vector< std::string_view > split_strict(StringType const& input, std::string const& delims)
{
	//The vector to contain the results
	std::vector< std::string_view > results;
	if (input.empty())
	{
		results.emplace_back();
		return results;
	}
	//The string was not empty, so end should be > start (contain at least 1 char)
	size_t start = 0;
	size_t end = input.size();
	//Getting the pointer to the first character of the string (note
	//it may be a delimiter).
	const char* ptr = input.data();

	//We can now loop de string starting from the position 0, look for the
	//character that is a delimiter, and create a string view for
	//the given range if it's valid.
	auto found = input.find_first_of(delims, start);
	while (found != std::string::npos && found < end)
	{
		results.emplace_back(ptr+start, found-start);
		//Find the next non delimiter character
		start = found+1;
		//starting from the next non-delimiter, try to find the next delimiter.
		found = input.find_first_of(delims, start);
	}

	//At this point we have 3 options
	//1) there was no delimiter at all on the string, ex "pepito". In that case the previous
	// loop was not executed at all, start is at 0 and should be previous to the end(empty strings
	// already returned).
	//2) There were some delimiters on the string, but after the lat one, none was found,
	// ex "pepito,juanito". In that case start is the position of the previously found delimiter+1
	//3) Especial case of previous case: there's no string after last delimiter. ex  "pepito,". For
	//that case again start is the position of last delimiter+1 which should be string.size() or
	//"one past the last item" which is the position of the end iterator.
	if (start <= end)
	{
		std::string_view last_token(ptr+start, end-start);
		results.push_back( last_token );
	}
	return results;

}

}

std::vector< std::string_view > split(std::string const& input)
{
	return detail::split_merge(input, " ");
}

std::vector< std::string_view > split(std::string_view const& input)
{
	return detail::split_merge(input, " ");
}

std::vector< std::string_view > split(std::string const& input, std::string const& delims)
{
	return detail::split_strict(input, delims);
}

std::vector< std::string_view > split(std::string_view const& input, std::string const& delims)
{
	return detail::split_strict(input, delims);
}

details::string_line_range slinerange(const std::string& s)
{
    return details::string_line_range(s);
}

details::string_line_range slinerange(const std::string_view s)
{
    return details::string_line_range(s);
}

}