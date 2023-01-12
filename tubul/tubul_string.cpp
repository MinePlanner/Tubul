//
// Created by Carlos Acosta on 12-01-23.
//

#include <vector>
#include <string>
#include <string_view>

namespace TU
{

std::vector< std::string_view > split(std::string const& input, std::string const& delims)
{
	//The vector to contain the results
	std::vector< std::string_view > results;
	if (input.empty())
		return results;
	//check the "real" begin and end in case there are delimiters at
	//the beginning or end of input.
	size_t start = 0, end = std::string::npos;
	//check if input only contains delimiters
	start = input.find_first_not_of(delims);
	if ( start == std::string::npos )
		return results;
	//Getting the pointer to the first character of the string (note
	//it may be a delimiter).
	const char* ptr = input.c_str();
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

}