//
// Created by Carlos Acosta on 30-03-23.
//
#include "tubul_file_utils.h"
#include "tubul_exception.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <charconv>
#include <fast_float/fast_float.h>

#ifndef TUBUL_WINDOWS
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace TU
{

//Simple wrapper for the common task of checking if a file exists
bool isRegularFile(const std::string_view& name) {
    return std::filesystem::is_regular_file(name);
}

//Several times we have needed to count a given character in a file, so
//this function wraps that task concisely.
size_t countCharInFile( const std::string_view& filename, char c)
{
    std::ifstream ifile(filename.data());
    auto start = std::istreambuf_iterator<char>(ifile);
    auto end = std::istreambuf_iterator<char>();

    return std::count(start, end, c);
}




//Template implementation to actually do the conversion to a given type.
//The idea is that here we do all the checks and pointer juggling of all
//the types just once, and provide specific named functions to the outside world
template <typename T>
inline T doCharConv(std::string_view s)
{
    auto [begin, end] = strview_range(s);
    T val;
    //If we are asked to parse a float, we have to use fast_float lib
    //until the floating point conversion gets into std.
    if constexpr ( std::is_floating_point_v<T>  ) {
        auto [ptr, ec] = fast_float::from_chars(begin, end, val);
        if (ec == std::errc() && ptr == end ) {
            return val;
        }
    }
    else if constexpr ( std::is_integral_v<T> )
    {
        auto [ptr, ec] = std::from_chars(begin, end, val);
        if (ec == std::errc() && ptr == end) {
            return val;
        }
    }
    throw std::invalid_argument{"Couldn't parse text correctly"};
}

double strToDouble(const std::string_view& p){
    return doCharConv<double>(p);
}

int strToInt(const std::string_view& p){
    return doCharConv<int>(p);
}

#ifndef TUBUL_WINDOWS
	MappedFile::MappedFile(const char* filename) {
		fd_ = open(filename, O_RDONLY );
		struct stat file_stats{0};
		if (fstat(fd_, &file_stats) == -1)
            throw TU::Exception(std::string("Could not open file:") + filename);

		size_ = file_stats.st_size;
		data_ = static_cast<char*>(  mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0) );
		//We expect to read the file sequentially.
		madvise(data_, size_, MADV_WILLNEED | MADV_SEQUENTIAL);
	}


  MappedFile::MappedFile(const std::string& filename) :
            MappedFile(filename.c_str())
            {}

  MappedFile::~MappedFile() {
		if (munmap(data_, size_) == -1)
		{
      //throw TU::Exception( "CAUTION!! I could not unmap the file properly");
      //We need to let the user know there was SOME error, but can't throw.
		}

		// Un-mmaping doesn't close the file, so we still need to do that.
		close(fd_);

	}
#endif 

}
