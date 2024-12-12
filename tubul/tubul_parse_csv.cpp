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
#include "tubul_types.h"
#include "tubul_parse_csv.h"
#include "tubul_logger.h"



namespace TU
{
  //
//Simple structure to hide the rapidcsv document from the headers.
struct CSVContents::CSVRawData
{
	explicit CSVRawData(std::string const& filename):
		doc(filename,rapidcsv::LabelParams(0,0) ){}

	explicit CSVRawData(std::istream& file_stream):
		doc(file_stream,rapidcsv::LabelParams(0,0)){}

	CSVRawData(std::istream& file_stream, rapidcsv::LabelParams labels, rapidcsv::SeparatorParams sep):
		doc(file_stream, labels, sep){}

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

CSVContents::CSVContents( std::unique_ptr<CSVRawData>&& rawData):
	impl_(std::move(rawData))
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
std::vector<int64_t> CSVContents::getColumnAsInteger(size_t colIndex) const {
	return impl_->doc.GetColumn<int64_t>(colIndex);
}
std::vector<std::string> CSVContents::getColumnAsString(size_t colIndex) const {
	return impl_->doc.GetColumn<std::string>(colIndex);
}
std::vector<std::string> CSVContents::getRow(size_t rowIndex) const {
	return impl_->doc.GetRow<std::string>(rowIndex);
}

std::optional <size_t> CSVContents::getColumnIndex(const std::string_view& name) const{
	std::vector <std::string> columns = impl_ -> doc.GetColumnNames();
	auto it = find(columns.begin(), columns.end(), name);
	if(it != columns.end()){
		return it - columns.begin();
	}
	return std::nullopt;
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


inline
std::optional<CSVContents> readCsv(std::istream& input, const CSVOptions& options)
{
	try{
		//Try to construct the CSV document directly in the optional.
		rapidcsv::LabelParams headers(
				(options.columnHeaders==ColumnHeaders::YES)?0:-1,
				(options.rowHeaders==RowHeaders::YES)?0:-1
							);
		rapidcsv::SeparatorParams separator(options.separator);
		auto raw = std::make_unique<CSVContents::CSVRawData>( input, headers, separator);

		std::optional<CSVContents> res(std::in_place, std::move(raw));
		return res;
	}
	catch (std::exception& ) {
		return std::nullopt;
	}
	catch (...) {
		return std::nullopt;
	}

}


inline
std::optional<CSVContents> readCsv(std::istream& input)
{
	try{
		//Try to construct the CSV document directly in the optional.
		std::optional<CSVContents> res(std::in_place, input);
		return res;
	}
	catch (std::exception& ) {
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
void setupDataFrameFromCSV(DataFrame& df, const rapidcsv::Document& doc)
{
	//Check what to do in case of no column names :/
	auto const& names = doc.GetColumnNames();
	for (auto const& name: names)
	{
		df.names_.emplace(name, doc.GetColumnIdx(name));
		df.type_.push_back( DataType::STRING );
		df.columns_.push_back( DataColumn() );
	}
}


//Helper function to retrieve the colum id's from a ColumnRequest. This
//can be more or less direct if the columns are requested by id, or may need
//to convert the names into id.
std::vector<size_t> getRequestedColumnId(const ColumnRequest& requestedColumns,  const rapidcsv::Document& doc)
{
	std::vector<size_t> res;

	const auto& requests_ = requestedColumns.requests_;

	if (std::holds_alternative<ColumnRequest::RequestsByName>(requests_))
	{
		const auto& reqs = std::get<ColumnRequest::RequestsByName>(requests_);
		for (const auto& req: reqs)
		{
			const auto& colName = req.first;
			res.push_back(doc.GetColumnIdx(colName) );
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
std::vector<size_t> getRequestedColumnId(const std::vector<std::string>& requestedColumns,  const rapidcsv::Document& doc)
{
	std::vector<size_t> res;
	res.reserve( requestedColumns.size());
	for (const auto& req: requestedColumns)
	{
		auto colIdx = doc.GetColumnIdx(req);
		res.push_back(colIdx);
	}

	return res;
}

//Function that will go over the columns defined in a dataframe and will try to
//populate them depending on if they were initialized and using the type already
//set to describe them. For now, only supporting doubles and strings as that
//should be the minimum to handle all other derived cases.
void getRequestedColumns(DataFrame& df,  const rapidcsv::Document& doc, std::vector<size_t>& columns)
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
			auto col = doc.GetColumn<double>(column_idx);
			colContainer[column_idx] = std::move(col);

		}
		else if (colType[column_idx] == DataType::INTEGER)
		{
			auto col = doc.GetColumn<int64_t>(column_idx);
			colContainer[column_idx] = std::move(col);
		}
		else if (colType[column_idx] == DataType::STRING)
		{
			auto col = doc.GetColumn<std::string>(column_idx);
			colContainer[column_idx] = std::move(col);
		}
	}

}

/** This function implements the process to read from a csv, setup a dataframe
 * and retrieve the selected columns into vectors. This is done by passing
 * the stream that contains the csv data + some csv options, and a couple
 * objects to customize the selection of columns and the expected types of
 * columns. Depending on what is asked, the parameters used to customize will
 * vary accordingly.
 * @param input stream where the CSV data can be retrieved from.
 * @param options CSV options, like existence of headers and separator character.
 * @param selector is a functor that should know how to return the list of column
 * id's that are being requested. This functor can wrap how the id's are obtained
 * for different cases, like selecting everything, or according to some customer
 * list with explicit id's or names or a weirder way (columns starting with "A"
 * could be possible for example).
 * @param typeRequestor is a functor that has to "fix" the types of each column in
 * the dataframe. This is done by setting the correct datatype value on the
 * corresponding column.
 * @return returns the dataframe
 */
template<typename ColSelector, typename ColTypeRequestor>
DataFrame dataFrameFromCSVInternal(std::istream& input, const CSVOptions& options, ColSelector& selector, ColTypeRequestor& typeRequestor)
{
	auto csv = readCsv(input, options);
	if ( !csv )
		throw std::runtime_error("couldn't read csv!");

	//Shorter name for the csv implementation.
	auto& contents = csv.value();

	//The result we are building.
	DataFrame df;

	//Setup the basics from the csv data: column count, names and assuming all types as string.
	setupDataFrameFromCSV(df, contents.impl_->doc);

	//Now, if we have type request information, fix the the dataframe types.
	typeRequestor( df );

	//Get get the index for columns that we are asked for. The selector will
	//vary depending on the needs of the caller.
	std::vector<size_t> columns = selector(contents.impl_->doc);

	getRequestedColumns(df, contents.impl_->doc, columns);

	return df;
}

/** Simple functor to return the id's of all columns from
 * a CSV document.
 */
struct SelectorAllColumns
{
	std::vector<size_t> operator()( const rapidcsv::Document& doc)
	{
		std::vector<size_t> columns;
		for( size_t it=0; it < doc.GetColumnCount(); ++it)
			columns.push_back(it);
		return columns;
	}
};

/** Object that will return only the id's of the columns that
 * are present on the provided vector of names.
 */
struct SelectorColumnByName
{
	explicit SelectorColumnByName(const std::vector<std::string>& names):
		names_(names)
	{}

	std::vector<size_t> operator()( const rapidcsv::Document& doc)
	{
		return getRequestedColumnId(names_, doc);
	}
	const std::vector<std::string>& names_;
};

/** Object that will return only the id's of the columns that
 * are present on the provided ColumnRequest object (that contains
 * info of name/id and types).
 */
struct SelectorColumnAndType
{
	SelectorColumnAndType(const ColumnRequest& request):
		request_(request)
	{}

	std::vector<size_t> operator()( const rapidcsv::Document& doc)
	{
		return getRequestedColumnId(request_, doc);
	}
	const ColumnRequest& request_;
};

/** Helper object to set type info on a dataframe when there's no info...
 * basically do nothing :D
 */
struct ColumnTypeNoInfo
{
	void operator()(DataFrame& )
	{ }

};

/** The ColumnRequest object contains type information for each column, and
 *this object can extract that information and pass it to the dataframe.
 */
struct ColumnTypeHelper
{
	explicit ColumnTypeHelper(const ColumnRequest& req):
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
			const auto& reqs = std::get<ColumnRequest::RequestsByPosition>(requests_);
			for (const auto& req: reqs)
			{
				const auto& colId = req.first;
				const auto& colType= req.second;
				df.type_[colId] = colType;
			}
		}

	}

	const ColumnRequest& request_;
};




//The following functions basically compose the previous objects and
//functions to provide the multiple overloads that allow different
//types of usages: retrieve all columns, only some, some typed columns, etc.



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
