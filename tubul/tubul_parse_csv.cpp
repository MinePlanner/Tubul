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
#include <regex>
#include <variant>
#include <algorithm>
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
		data_ = static_cast<char*>(  mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0) );
		//We expect to read the file sequentially.
		madvise(data_, size_, MADV_WILLNEED | MADV_SEQUENTIAL);
	}

	[[nodiscard]]
	char* data() const { return data_;}
	[[nodiscard]]
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

struct CSVContents::CSVColumns
{
	using DoubleColumn = std::vector<double>;
	using IntegerColumn = std::vector<long>;
	using StringColumn = std::vector<std::string>;
	using DataColumn = std::variant<DoubleColumn, IntegerColumn, StringColumn>;


	std::vector<ColumnType> type;
	std::vector<DataColumn> col;
};


//Constructors for the CSVContents object that will handle the
//data read from a CSV file.
CSVContents::CSVContents(const std::string &filename):
	impl_(std::make_unique<CSVContents::CSVImpl>(filename)),
	cols_(std::make_unique<CSVColumns>())
{}

CSVContents::CSVContents(std::istream& input_stream):
	impl_(std::make_unique<CSVContents::CSVImpl>(input_stream)),
	cols_(std::make_unique<CSVColumns>())
{}

CSVContents::CSVContents(CSVContents &&other):
	impl_(std::move(other.impl_)),
	cols_(std::move(other.cols_))
{}

CSVContents::~CSVContents() = default;

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
std::vector<std::string> CSVContents::getRow(size_t rowIndex) const {
	return impl_->doc.GetRow<std::string>(rowIndex);
}

void guess_column_types(CSVContents& csv)
{
	using ColumnType = CSVContents::ColumnType;

	const auto num_cols = csv.colCount();

	//the regex to recognize integer and doubles.
	std::regex int_nums("[\\-]?[0-9]+");
	std::regex sci_nums("[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+\\-]?\\d+)?");

	//This should be improved to sample rows farther away. Maybe things like
	//some from begining, middle and end of list plus some other random locations.
	auto get_sample_rows = [](CSVContents const& csv){ return std::vector<size_t>{1,2,3}; };
	//We will check these rows (and we should check a number bigger than 0!)
	auto rows_to_check = get_sample_rows(csv);
	assert( rows_to_check.size() > 0);
	auto& types = csv.cols_->type;

	//trying to guess the type of the columns by detecting types using regexes. This
	//will have one vector of detected types PER COLUMN, and each vector should have as
	//guesses as the number of rows investigated.
	std::vector<std::vector<CSVContents::ColumnType> > detected_types(num_cols );
	for (auto const& n: rows_to_check)
	{
		auto this_row = csv.getRow(n);
		size_t col_count = 0;
		for (auto const &item : this_row) {
			auto& col_types = detected_types[col_count];
			std::smatch cm;
			if ( std::regex_match( item, cm, int_nums) ) {
				col_types.push_back(ColumnType::INTEGER);
			}
			else if ( std::regex_match(item, cm, sci_nums) ) {
				col_types.push_back(ColumnType::DOUBLE);
			}
			else {
				col_types.push_back(ColumnType::STRING);
			}
			col_count++;
		}
	}

	//Now, consider the guesses from looking at the results from different rows.
	for (auto col_idx = 0; col_idx < num_cols; ++col_idx)
	{
		//Strictness (from higher to lower) Integer > Double > String
		auto const& guess_for_this_col = detected_types[col_idx];
		//If we only checked a single row, that's the type we are assigning.
		if ( rows_to_check.size() == 1)
		{
			types.push_back(guess_for_this_col.front());
			continue;
		}
		else
		{//Check if all the rows got to the same conclusion, and simply assign.
			auto first_guess = guess_for_this_col.front();
			bool all_same = true;
			for ( auto guess: guess_for_this_col) {
				if ( first_guess != guess ){
					all_same = false;
					break;
				}
			}
			if (all_same)
				types.push_back(first_guess);
			else
				std::cout << "there was a disagreement for type of column: " << col_idx << std::endl;
		}


	}

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

		std::cout << std::endl;
		//-> Using an istream that uses a custom buffer.
		std::istream file_stream(&buf );
		//-> Direct approach using an ifstream.
// 		std::ifstream file_stream(filename);
		//Try to construct the CSV document directly in the optional.
		std::optional<CSVContents> res(std::in_place, file_stream);
		auto& csv_file = *res;
		std::cout << "Rows detected: " <<  csv_file.rowCount() << std::endl;
		std::cout << "Columns detected: " <<  csv_file.colCount() << std::endl;
		auto names =  csv_file.impl_->doc.GetColumnNames();
		//fill the column type hint for this file columns.
		guess_column_types(csv_file );

		auto& col_type = csv_file.cols_->type;
		for (int i = 0; i < csv_file.colCount(); ++i)
		{
			if (col_type[i] == CSVContents::ColumnType::INTEGER)
			{
				std::cout << "Getting integer column " << names[i] << std::endl;
				auto col = csv_file.getColumnAsInteger(i);
				csv_file.cols_->col.emplace_back(std::move(col));

			}
			else if (col_type[i] == CSVContents::ColumnType::DOUBLE)
			{
				std::cout << "Getting double column " << names[i] << std::endl;
				auto col = csv_file.getColumnAsDouble(i);
				csv_file.cols_->col.emplace_back(std::move(col));
			}
			else
			{
				std::cout << "Getting string column " << names[i] << std::endl;
				auto col = csv_file.getColumnAsString(i);
				csv_file.cols_->col.emplace_back(col);
			}
		}


		return std::move(res);
	}
	catch (std::exception& e) {
		return std::nullopt;
	}
	catch (...) {
		return std::nullopt;
	}


}


}