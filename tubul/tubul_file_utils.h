//
// Created by Carlos Acosta on 04-05-23.
//

#pragma once
#include <string_view>
#include <tuple>
#include <memory>
#include <streambuf>
#include <fstream>
#include "tubul_string.h"

namespace TU{
/** Set of simple functions to perform tasks that are common
 * but regularly require typing a lot more than just these names
 * in order to do them. For example the strToXXX use the from_chars functions
 * which are awesome, but require a couple extra steps to be used which are
 * very cumbersome.
 */
    bool isRegularFile(const std::string_view& name);
    size_t countCharInFile( const std::string_view& filename, char c);
    double strToDouble(const std::string_view& p);
    int strToInt(const std::string_view& p);


//Simple wrapper for common task when using str_views to convert to types.
    constexpr auto strview_range(std::string_view s) noexcept {
        return std::make_tuple(s.data(), s.data() + s.size());
    }



//Structure to hold a Memory map of a file. For some cases,
//it's very useful to have access to a file a memory map instead
//of the normal c++ stream view.
struct MappedFile
{
    struct Internals;

	explicit MappedFile(const char* filename);
    explicit MappedFile(const std::string& filename);

	[[nodiscard]]
	const char* data() const { return data_;}
	[[nodiscard]]
	size_t size() const { return size_;}
    [[nodiscard]]
    std::string_view string_view() const { return {data_, size_};}

	~MappedFile();

	const char* data_;
	size_t size_;
    std::unique_ptr<Internals> impl_;
};

//This is just a simple wrapper for the cumbersome way to read
//the contents of a file into a string.
inline
std::string readToString(const std::string& filename)
{
	std::ifstream ifile( filename );
	std::string str((std::istreambuf_iterator<char>(ifile)),
					 std::istreambuf_iterator<char>());
	return str;
}


template<typename Lambda_t>
void readFileLines(const std::string_view &filename, Lambda_t lambda)
{
    MappedFile input(filename.data());
    auto file_contents = input.string_view();

    for (auto line : slinerange(file_contents))
    {
        lambda(line);
    }
} 

}
