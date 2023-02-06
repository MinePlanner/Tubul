//
// Created by Carlos Acosta on 02-02-23.
//

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <strstream>
#include <streambuf>
#include <rapidcsv.h>
#include "tubul_parse_csv.h"

namespace TU
{

//Structure to hold a Memory map of a file. For some cases,
//it's very useful to have access to a file a memory map instead
//of the normal c++ stream view.
struct MappedFile
{
	explicit MappedFile(const char* filename)
	{
		fd_ = open(filename, O_RDONLY );
		struct stat file_stats{0};
		if (fstat(fd_, &file_stats) == -1)
		{
			//ERROR handling
			std::cout << "This is bad!!" << std::endl;
		}
		size_ = file_stats.st_size;
		data_ = static_cast<char*>(  mmap(NULL, size_, PROT_READ, MAP_PRIVATE, fd_, 0) );
		//We expect to read the file sequentially.
		madvise(data_, size_, MADV_WILLNEED | MADV_SEQUENTIAL);
	}

	char* data() const { return data_;}
	std::streamsize size() const { return size_;}

	~MappedFile()
	{
		if (munmap(data_, size_) == -1)
		{
			//Do some error handling? Likely just log and get out
		}

		// Un-mmaping doesn't close the file, so we still need to do that.
		close(fd_);

	}
	int fd_;
	char* data_;
	std::streamsize size_;
};

//Simple structure to hide the rapidcsv document from the headers.
struct CSVContents::CSVImpl
{
	explicit CSVImpl(std::string const& filename):
		doc(filename){}

	explicit CSVImpl(std::istream& file_stream):
		doc(file_stream){}

	rapidcsv::Document doc;
};
//Constructors for the CSVContents object that will handle the
//data read from a CSV file.
CSVContents::CSVContents(const std::string &filename):
	impl_(std::make_unique<CSVContents::CSVImpl>(filename))
{}

CSVContents::CSVContents(std::istream& input_stream):
	impl_(std::make_unique<CSVContents::CSVImpl>(input_stream))
{}

CSVContents::CSVContents(CSVContents &&other):
	impl_(std::move(other.impl_))
{}

CSVContents::~CSVContents() {}

//Implementation of these methods is a simple passthrough to
//the internal rapidcsv implementation.
size_t CSVContents::rowCount() const {return impl_->doc.GetRowCount();}
size_t CSVContents::colCount() const {return impl_->doc.GetColumnCount();}
std::vector<std::string> CSVContents::getColNames() const { return impl_->doc.GetColumnNames();}
std::vector<double> CSVContents::getColumnAsDouble(size_t colIndex) const {
	return impl_->doc.GetColumn<double>(colIndex);
}
std::vector<long> CSVContents::getColumnAsInteger(size_t colIndex) const {
	return impl_->doc.GetColumn<long>(colIndex);
}
std::vector<std::string> CSVContents::getColumnAsString(size_t colIndex) const {
	return impl_->doc.GetColumn<std::string>(colIndex);
}

//This is a more "real method", but still the rapidcsv lib is the one
//doing all the heavy lifting. We try to read the file using mmap
//and then parse the csv using rapidcsv.
std::optional<CSVContents> read_csv(const std::string& filename)
{
	try{
		//Calling the file mapping stuff. With this, we have the file
		//open in a c-style, with the contents mmaped and ready to be read.
		TU::MappedFile da_file(filename.c_str());
		//std::stringbuf buf(std::ios_base::in);
		//buf.pubsetbuf(da_file.data(), da_file.size());
		//buffer is an strstreambuf which is exactly what we need (treats an
		//array of chars as a source for the buffer).
		std::strstreambuf buf(da_file.data(), da_file.size());
		//-> Using a istream that uses a custom buffer.
		std::istream file_stream(&buf );
		//-> Direct approach using an ifstream.
// 		std::ifstream file_stream(filename);
		//Try to construct the CSV document directly in the optional.
		std::optional<CSVContents> res(std::in_place, file_stream);
		auto& csv_file = *res;
		std::cout << "Columns detected: " <<  csv_file.colCount() << std::endl;
		std::cout << "Rows detected: " <<  csv_file.rowCount() << std::endl;
		size_t col_index = 0;
		for (const auto& col: csv_file.impl_->doc.GetColumnNames())
			std::cout << "Col("<<col_index++<<"):" << col <<std::endl;

		return std::move(res);
	}
	catch (std::exception& e) { return std::nullopt; }
	catch (...) { return std::nullopt; }


}


}