//
// Created by Carlos Acosta on 02-02-23.
//

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fast_float/fast_float.h>
#include <rapidcsv.h>

#include <iostream>
#include <streambuf>
#include <regex>
#include <algorithm>
#include "tubul_types.h"
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
			std::cout << "CAUTION!! I could not unmap the file properly" << std::endl;
		}

		// Un-mmaping doesn't close the file, so we still need to do that.
		close(fd_);

	}
	int fd_;
	char* data_;
	std::streamsize size_;
};

//Simple structure to hide the rapidcsv document from the headers.
struct CSVContents::CSVRawData
{
	explicit CSVRawData(std::string const& filename):
		doc(filename,rapidcsv::LabelParams(0,0) ){}

	explicit CSVRawData(std::istream& file_stream):
		doc(file_stream,rapidcsv::LabelParams(0,0)){}

	rapidcsv::Document doc;
};

//Easy retrieval of columns from the set of clumns that we have.
const DataColumn& CSVColumns::operator[](size_t idx) const
{
	if ( idx > columns_.size() ||
		std::holds_alternative<std::monostate>(columns_[idx]) )
		throw std::runtime_error("Requesting invalid column");
	return columns_.at(idx);
}
const DataColumn& CSVColumns::operator[](const std::string& name) const
{
	auto idx = names_.at(name);
	return operator[](idx);
}


//Constructors for the CSVContents object that will handle the
//data read from a CSV file.
CSVContents::CSVContents(const std::string &filename):
	impl_(std::make_unique<CSVContents::CSVRawData>(filename))
{}

CSVContents::CSVContents(std::istream& input_stream):
	impl_(std::make_unique<CSVContents::CSVRawData>(input_stream))
{}

CSVContents::CSVContents(CSVContents &&other) noexcept:
	impl_(std::move(other.impl_))
{}

CSVContents& CSVContents::operator=(CSVContents &&other) noexcept
{
	impl_ = std::move(other.impl_);
	return *this;
}

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


//Function with the logic to "auto-detect" types of columns. Basically
//This tries to get a couple of rows from the CSV and tries to match
//different tokens as intenger or doubles (either normal or scientific).
//If they fail, the column is just parsed as strings.
//To review later: a clever way to find rows to use as sample.
std::vector<DataType> guess_column_types(CSVContents& csv)
{
	const auto num_cols = csv.colCount();

	//the regex to recognize integer and doubles.
	std::regex int_nums(R"([\-]?[0-9]+)");
	std::regex sci_nums(R"([+\-]?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+\-]?\d+)?)");

	//This should be improved to sample rows farther away. Maybe things like
	//some from begining, middle and end of list plus some other random locations.
	auto get_sample_rows = [](CSVContents const& csv){ return std::vector<size_t>{0,1,2}; };
	//We will check these rows (and we should check a number bigger than 0!)
	auto rows_to_check = get_sample_rows(csv);
	assert( rows_to_check.size() > 0);
	std::vector<DataType> types ;

	//trying to guess the type of the columns by detecting types using regexes. This
	//will have one vector of detected types PER COLUMN, and each vector should have as
	//guesses as the number of rows investigated.
	std::vector<std::vector<TU::DataType> > detected_types(num_cols );
	for (auto const& n: rows_to_check)
	{
		auto this_row = csv.getRow(n);
		size_t col_count = 0;
		for (auto const &item : this_row) {
			auto& col_types = detected_types[col_count];
			std::smatch cm;
			if ( std::regex_match( item, cm, int_nums) ) {
				col_types.push_back(DataType::INTEGER);
			}
			else if ( std::regex_match(item, cm, sci_nums) ) {
				col_types.push_back(DataType::DOUBLE);
			}
			else {
				col_types.push_back(DataType::STRING);
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
	return types;

}

//Helper function to request conversion to column format for _all_ the columns of a
//given file.
CSVColumns CSVContents::convertAllToColumnFormat()
{
	auto const names =  getColNames();

	auto const& doc = impl_->doc;
	std::vector<size_t> column_indices;
	for (auto const& name: names)
		column_indices.push_back(doc.GetColumnIdx(name));

	return convertToColumnFormat(column_indices);
}

//Helper function to request conversion to column format for specific columns from a
//given file.
CSVColumns CSVContents::convertToColumnFormat(std::vector<std::string> const& columns)
{
	std::vector<size_t> column_indices;
	column_indices.reserve(columns.size());
	for (auto const& column_name: columns) {
		column_indices.push_back(impl_->doc.GetColumnIdx(column_name));
	}
	return convertToColumnFormat(column_indices);
}

//Main function to get the data of the CSV as columns for the requested
//values. The other functions should eventually call this one. This
//function will create a CSVColumns object that stores the data as vector
//for easier/faster handling. Do note that the CSVColumns object is
//independent of the original Contents it was created from, although it will
//retain the column names and indices.
CSVColumns CSVContents::convertToColumnFormat(std::vector<size_t> const& columns)
{
	//The result we are building.
	CSVColumns cols;
	//But if we are asked for nothing.... that's it :D
	if ( columns.empty() )
		return cols;

	if (cols.type_.empty())
		cols.type_ = guess_column_types(*this);

	auto& col_container = cols.columns_;
	//We should have as many columns as guessed datatypes
	if (col_container.size() != cols.type_.size() )
		col_container.resize(cols.type_.size());

	auto const& names = impl_->doc.GetColumnNames();
	for (auto const& name: names)
		cols.names_.emplace(name, impl_->doc.GetColumnIdx(name));

	auto const& col_type = cols.type_;
	for (auto column_idx: columns)
	{
		//Just in case, check we didn't already get this one if a column was repeated
		//or got called twice.
		if ( not holds_alternative<std::monostate>(col_container[column_idx]))
			continue;

		//Retrieve the column according to the discovered type.
		if (col_type[column_idx] == DataType::INTEGER)
		{
			auto col = getColumnAsInteger(column_idx);
			col_container[column_idx] = std::move(col);

		}
		else if (col_type[column_idx] == DataType::DOUBLE)
		{
			auto col = getColumnAsDouble(column_idx);
			col_container[column_idx] = std::move(col);
		}
		else
		{
			//Ignoring string column
		}

	}
	return cols;
}


//This is the  "real method", but still the rapidcsv lib is the one
//doing all the heavy lifting. We try to read the file using mmap
//and then parse the csv using rapidcsv.
std::optional<CSVContents> readCsv(const std::string& filename)
{
	try{
#if 0
		//Calling the file mapping stuff. With this, we have the file
		//open in a c-style, with the contents mmaped and ready to be read.
		TU::MappedFile da_file(filename.c_str());
		//buffer is an strstreambuf which is exactly what we need (treats an
		//array of chars as a source for the buffer).
		std::strstreambuf buf(da_file.data(), da_file.size());
		//-> Using an istream that uses a custom buffer.
		std::istream file_stream(&buf );
#endif
		//-> Direct approach using an ifstream.
 		std::ifstream file_stream(filename);
		//Try to construct the CSV document directly in the optional.
		std::optional<CSVContents> res(std::in_place, file_stream);

		return std::move(res);
	}
	catch (std::exception& e) {
		return std::nullopt;
	}
	catch (...) {
		return std::nullopt;
	}


}

//and then read from it as if it was a file. Pretty useful for testing or small
//experiments.
std::optional<CSVContents> readCsvFromString(const std::string& contents)
{
	try{
		//-> Direct approach using an ifstream.
		std::stringstream contents_stream(contents);
		//Try to construct the CSV document directly in the optional.
		std::optional<CSVContents> res(std::in_place, contents_stream);

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