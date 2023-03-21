//
// Created by Carlos Acosta on 02-02-23.
//

#include <fast_float/fast_float.h>
#include <rapidcsv.h>
#include <tuple>
#include <iostream>
#include <streambuf>
#include <optional>
#include <regex>
#include <algorithm>
#include "tubul_types.h"
#include "tubul_parse_csv.h"
#ifndef TUBUL_WINDOWS
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif


namespace TU
{
#ifndef TUBUL_WINDOWS
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
#endif 
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
const DataColumn& DataFrame::operator[](size_t idx) const
{
	if ( idx > columns_.size() ||
		std::holds_alternative<std::monostate>(columns_[idx]) )
		throw std::runtime_error("Requesting invalid column");
	return columns_.at(idx);
}
const DataColumn& DataFrame::operator[](const std::string& name) const
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


ColumnRequest& ColumnRequest::add(size_t columnIndex, TU::DataType type)
{
	//It has not been initialized.
	if (std::holds_alternative<std::monostate>(requests_))
	{
		requests_ = RequestsByPosition{ {columnIndex, type} };
	}
	else if ( std::holds_alternative<RequestsByName>(requests_))
	{
		throw std::runtime_error("Can't hold named and indexed requests at the same time");
	}
	else
	{
		std::get<RequestsByPosition>( requests_).emplace_back(columnIndex, type);
	}

	return *this;
}

ColumnRequest& ColumnRequest::add(const std::string& columnName, TU::DataType type)
{
	if (std::holds_alternative<std::monostate>(requests_))
	{
		requests_ = RequestsByName{ {columnName, type} };
	}
	else if ( std::holds_alternative<RequestsByPosition>(requests_))
	{
		throw std::runtime_error("Can't hold named and indexed requests at the same time");
	}
	else
	{
		std::get<RequestsByName>( requests_).emplace_back(columnName, type);
	}
	return *this;
}


size_t ColumnRequest::size() const
{
	if (std::holds_alternative<std::monostate>(requests_))
		return 0;
	if (std::holds_alternative<RequestsByPosition>(requests_))
		return std::get<RequestsByPosition>(requests_).size();
	if (std::holds_alternative<RequestsByName>(requests_))
		return std::get<RequestsByName>(requests_).size();
	//default to no requests.
	return 0;
}


size_t DataFrame::getColCount() const
{
	return columns_.size();
}

struct ColSizeVisitor
{
	size_t operator()(const TU::DoubleColumn& c){ return c.size();}
	size_t operator()(const TU::StringColumn& c){ return c.size();}
	template <typename InvalidColumnType>
	size_t operator()(const InvalidColumnType&){ throw std::runtime_error("invalid column type");}
};

size_t DataFrame::getRowCount() const
{
	for (const auto& column: columns_)
	{
		if ( !std::holds_alternative<std::monostate>(column))
			return std::visit( ColSizeVisitor() ,column);
	}
	return 0;
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
//FXIME: Decidir si borramos esto o no.
DataFrame CSVContents::convertAllToColumnFormat()
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
//FXIME: Decidir si borramos esto o no.
DataFrame CSVContents::convertToColumnFormat(std::vector<std::string> const& columns)
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
//function will create a DataFrame object that stores the data as vector
//for easier/faster handling. Do note that the DataFrame object is
//independent of the original Contents it was created from, although it will
//retain the column names and indices.
//FXIME: Decidir si borramos esto o no.
DataFrame CSVContents::convertToColumnFormat(std::vector<size_t> const& columns)
{
	//The result we are building.
	DataFrame cols;
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

inline
std::optional<CSVContents> readCsv(std::istream& input)
{
	try{
		//Try to construct the CSV document directly in the optional.
		std::optional<CSVContents> res(std::in_place, input);
		return std::move(res);
	}
	catch (std::exception& e) {
		return std::nullopt;
	}
	catch (...) {
		return std::nullopt;
	}

}

//This is the "real method", but still the rapidcsv lib is the one
//doing all the heavy lifting. We try to read the file using mmap
//and then parse the csv using rapidcsv.
std::optional<CSVContents> readCsv(const std::string& filename)
{
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
	return readCsv(file_stream);
}

//and then read from it as if it was a file. Pretty useful for testing or small
//experiments.
std::optional<CSVContents> readCsvFromString(const std::string& contents)
{
	//->Creating a stringstream to read from the string.
	std::stringstream contents_stream(contents);
	return readCsv(contents_stream);
}



//We fill the dataframe with very basic data from the csv. We copy the column names
//
void setupDataFrameFromCSV(DataFrame& frame, const std::unique_ptr<CSVContents::CSVRawData>& ptr)
{
	//Check what to do in case of no column names :/
	auto const& names = ptr->doc.GetColumnNames();
	for (auto const& name: names)
	{
		frame.names_.emplace(name, ptr->doc.GetColumnIdx(name));
		frame.type_.push_back( DataType::STRING );
		frame.columns_.push_back( DataColumn() );
	}
}

void setupDataFrameRequestedTypes(DataFrame& cols, const ColumnRequest& requestedColumns)
{
	if (requestedColumns.size() == 0)
		return;

	const auto& requests_ = requestedColumns.requests_;

	if (std::holds_alternative<ColumnRequest::RequestsByName>(requests_))
	{
		const auto& reqs = std::get<ColumnRequest::RequestsByName>(requests_);
		for (const auto& req: reqs)
		{
			const auto& colName = req.first;
			const auto& colType= req.second;
			size_t colId = cols.names_[colName];
			cols.type_[colId] = colType;
		}
	}
	if (std::holds_alternative<ColumnRequest::RequestsByPosition>(requests_))
	{
		const auto& reqs = std::get<ColumnRequest::RequestsByPosition>(requests_);

	}

}

//Helper function to retrieve the colum id's from a ColumnRequest. This
//can be more or less direct if the columns are requested by id, or may need
//to convert the names into id.
std::vector<size_t> getRequestedColumnId(const ColumnRequest& requestedColumns,  const std::unique_ptr<CSVContents::CSVRawData>& ptr)
{
	std::vector<size_t> res;

	const auto& requests_ = requestedColumns.requests_;

	if (std::holds_alternative<ColumnRequest::RequestsByName>(requests_))
	{
		const auto& reqs = std::get<ColumnRequest::RequestsByName>(requests_);
		for (const auto& req: reqs)
		{
			const auto& colName = req.first;
			res.push_back(ptr->doc.GetColumnIdx(colName) );
		}
	}
	else if (std::holds_alternative<ColumnRequest::RequestsByPosition>(requests_))
	{
		const auto& reqs = std::get<ColumnRequest::RequestsByPosition>(requests_);
		for ( auto idx: reqs)
			res.push_back(idx.first);
	}

	return res;
}

//Helper function to retrieve the ids of a list of column names.
std::vector<size_t> getRequestedColumnId(const std::vector<std::string>& requestedColumns,  const std::unique_ptr<CSVContents::CSVRawData>& ptr)
{
	std::vector<size_t> res;
	res.reserve( requestedColumns.size());
	for (const auto& req: requestedColumns)
	{
		auto colIdx = ptr->doc.GetColumnIdx(req);
		res.push_back(colIdx);
	}

	return res;
}

//Function that will go over the columns defined in a dataframe and will try to
//populate them depending on if they were initialized and using the type already
//set to describe them. For now, only supporting doubles and strings as that
//should be the minimum to handle all other derived cases.
void getRequestedColumns(DataFrame& df,  const std::unique_ptr<CSVContents::CSVRawData>& ptr, std::vector<size_t>& columns)
{
	auto& colContainer = df.columns_;
	auto const& colType = df.type_;

	for (auto column_idx: columns )
	{
		//Just in case, check we didn't already get this one if a column was repeated
		//or got called twice.
		if ( not holds_alternative<std::monostate>(colContainer[column_idx]))
			continue;

		//Retrieve the column according to the discovered type.
		if (colType[column_idx] == DataType::DOUBLE)
		{
			auto col = ptr->doc.GetColumn<double>(column_idx);
			colContainer[column_idx] = std::move(col);

		}
		else if (colType[column_idx] == DataType::STRING)
		{
			auto col = ptr->doc.GetColumn<std::string>(column_idx);
			colContainer[column_idx] = std::move(col);
		}
	}

}

//Helper object to set type info on a dataframe when there's no info...
//basically do nothing :D
struct ColumnTypeNoInfo
{
	void operator()(DataFrame& df)
	{ }

};

//The ColumnRequest object contains type information for each column, and
//this object can extract that information and pass it to the dataframe.
struct ColumnTypeHelper
{
	ColumnTypeHelper(const ColumnRequest& req):
		request_(req)
	{}

	void operator()(DataFrame& df)
	{
		if (request_.size() == 0)
			return;

		const auto& requests_ = request_.requests_;

		if (std::holds_alternative<ColumnRequest::RequestsByName>(requests_))
		{
			const auto& reqs = std::get<ColumnRequest::RequestsByName>(requests_);
			for (const auto& req: reqs)
			{
				const auto& colName = req.first;
				const auto& colType= req.second;
				size_t colId = df.names_[colName];
				df.type_[colId] = colType;
			}
		}
		if (std::holds_alternative<ColumnRequest::RequestsByPosition>(requests_))
		{
			const auto& reqs = std::get<ColumnRequest::RequestsByPosition>(requests_); //Fixme!
		}

	}

	const ColumnRequest& request_;
};

struct SelectorAllColumns
{
	std::vector<size_t> operator()( const std::unique_ptr<CSVContents::CSVRawData>& csv)
	{
		std::vector<size_t> columns;
		for( int it=0; it < csv->doc.GetColumnCount(); ++it)
			columns.push_back(it);
		return columns;
	}
};

struct SelectorColumnByName
{
	SelectorColumnByName(const std::vector<std::string>& names):
		names_(names)
	{}

	std::vector<size_t> operator()( const std::unique_ptr<CSVContents::CSVRawData>& csv)
	{
#if 0
		std::vector<size_t> columns;
		for (const std::string& name: names_)
			columns.push_back( csv->doc.GetColumnIdx(name) );

		return columns;
#endif
		return getRequestedColumnId(names_, csv);
	}
	const std::vector<std::string>& names_;
};

struct SelectorColumnAndType
{
	SelectorColumnAndType(const ColumnRequest& request):
		request_(request)
	{}

	std::vector<size_t> operator()( const std::unique_ptr<CSVContents::CSVRawData>& csv)
	{
		return getRequestedColumnId(request_, csv);
	}
	const ColumnRequest& request_;
};


//Main function to read from a csv, setup a dataframe and retrieve the selected
//columns into vectors. This is done by passing a the stream that contains
//the csv data, and a couple objects to customize the selection of columns
//and the expected types of columns. Depending on what is asked, the parameters
//used to customize will vary accordingly.
//@param selector is a functor that should know how to return the list of column
//id's that are being requested.
//@param typeRequestor is a functor that has to "fix" the types of each column
//in the dataframe. This is done by setting the correct datatype value on the
//corresponding column.
template<typename ColSelector, typename ColTypeRequestor>
DataFrame dataFrameFromCSVInternal(std::istream& input, const CSVOptions& options, ColSelector& selector, ColTypeRequestor& typeRequestor)
{
	auto csv = readCsv(input);
	if ( !csv )
		throw std::runtime_error("couldn't read csv!");

	//Shorter name for the csv implementation.
	auto& contents = csv.value();

	//The result we are building.
	DataFrame df;

	//Setup the basics from the csv data: column count, names and assuming all types as string.
	setupDataFrameFromCSV(df, contents.impl_);

	//Now, if we have type request information, fix the the dataframe types.
	typeRequestor( df );

	//Get get the index for columns that we are asked for. The selector will
	//vary depending on the needs of the caller.
	std::vector<size_t> columns = selector(contents.impl_);

	getRequestedColumns(df, contents.impl_, columns);

	return df;
}

DataFrame dataFrameFromCSVString(const std::string& csvContents, TU::CSVOptions options)
{
	std::stringstream contents_stream(csvContents);
	SelectorAllColumns all;
	ColumnTypeNoInfo noInfo;
	return dataFrameFromCSVInternal( contents_stream, options, all, noInfo );
}

DataFrame dataFrameFromCSVString(const std::string& csvContents, const ColumnRequest& requestedColumns, TU::CSVOptions options)
{
	std::stringstream contents_stream(csvContents);
	SelectorColumnAndType chosenCols(requestedColumns);
	ColumnTypeHelper noInfo(requestedColumns);

	return dataFrameFromCSVInternal(contents_stream, options, chosenCols, noInfo );
}

DataFrame dataFrameFromCSVString(const std::string& csvContents, const std::vector<std::string>& requestedColumns, TU::CSVOptions options)
{
	std::stringstream contents_stream(csvContents);
	SelectorColumnByName chosenCols(requestedColumns);
	ColumnTypeNoInfo typeInfo;
	return dataFrameFromCSVInternal( contents_stream, options, chosenCols, typeInfo );
}

DataFrame dataframeFromCSVFile(const std::string& filename, TU::CSVOptions options)
{
	std::ifstream contents_stream(filename);
	SelectorAllColumns all;
	ColumnTypeNoInfo noInfo;
	return dataFrameFromCSVInternal( contents_stream, options, all, noInfo );
}

DataFrame dataFrameFromCSVFile(const std::string& filename, const std::vector<std::string>& requestedColumns, TU::CSVOptions options)
{
	std::ifstream contents_stream(filename);
	SelectorColumnByName chosenCols(requestedColumns);
	ColumnTypeNoInfo noInfo;
	return dataFrameFromCSVInternal( contents_stream, options, chosenCols, noInfo );
}

DataFrame dataFrameFromCSVFile(const std::string& filename, const ColumnRequest& requestedColumns, TU::CSVOptions options)
{
	std::ifstream contents_stream(filename);
	SelectorColumnAndType chosenCols(requestedColumns);
	ColumnTypeHelper typeInfo(requestedColumns);

	return dataFrameFromCSVInternal(contents_stream, options, chosenCols, typeInfo );
}

}