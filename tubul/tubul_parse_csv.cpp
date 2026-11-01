//
// Created by Carlos Acosta on 02-02-23.
//

#include <fast_float/fast_float.h>
#include <rapidcsv.h>
#include <charconv>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <optional>
#include <span>
#include <string_view>
#include "tubul_types.h"
#include "tubul_parse_csv.h"
#include "tubul_csv_line_splitter.h"
#include "tubul_string.h"
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
	size_t operator()(const TU::IntegerColumn& c){ return c.size();}
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


//The dataframe functions below don't use rapidcsv: they read the input once,
//line by line (CSVLineSplitter, which splits fields with rapidcsv's rules), and
//keep only the requested columns, already converted to their type.
namespace
{

//A column of the dataframe that is filled: where it is in each line, and the
//vector its values go to.
struct FilledColumn
{
	size_t field_;
	std::string name_;
	DoubleColumn* doubles_ = nullptr;
	IntegerColumn* integers_ = nullptr;
	StringColumn* strings_ = nullptr;
};

//Throws the error for a value that can't be read as the column's type, naming
//the line, the column and the text.
[[noreturn]] void throwBadValue(const FilledColumn& column, std::string_view text, size_t lineNumber, const char* expected)
{
	throw std::runtime_error("CSV line " + std::to_string(lineNumber) + ", column '" + column.name_ + "': '" +
							 std::string(text) + "' is not " + expected);
}

//As rapidcsv, which used fast_float: the number is read from the start of the
//text, and characters after it are ignored.
double toDouble(const FilledColumn& column, std::string_view text, size_t lineNumber)
{
	double value = 0.0;
	const auto res = fast_float::from_chars(text.data(), text.data() + text.size(), value);
	if (res.ec != std::errc())
		throwBadValue(column, text, lineNumber, "a number");
	return value;
}

//As rapidcsv, which used std::stol: leading whitespace and a '+' sign are
//accepted, and characters after the number are ignored.
int64_t toInteger(const FilledColumn& column, std::string_view text, size_t lineNumber)
{
	std::string_view number = ltrim(text);
	if (!number.empty() && number.front() == '+')
	{
		number.remove_prefix(1);
		//As std::stol: a '+' can't be followed by another sign.
		if (!number.empty() && number.front() == '-')
			throwBadValue(column, text, lineNumber, "an integer");
	}
	int64_t value = 0;
	const auto res = std::from_chars(number.data(), number.data() + number.size(), value);
	if (res.ec != std::errc())
		throwBadValue(column, text, lineNumber, "an integer");
	return value;
}

//The dataframe's names, types and empty columns, from the header line (or from
//the first data line, when there are no column headers). With row headers, the
//first field of each line names the row and isn't a column.
struct DataFrameLayout
{
	DataFrame df_;
	std::vector<std::string> names_;
	size_t firstField_ = 0;
	bool hasColumnNames_ = false;
};

//Lays out the dataframe from the first line: its fields are the column names,
//or only count the columns when there are no column headers.
DataFrameLayout layoutDataFrame(std::span<const std::string_view> firstLine, const CSVOptions& options)
{
	DataFrameLayout layout;
	layout.firstField_ = (options.rowHeaders == RowHeaders::YES) ? 1 : 0;
	layout.hasColumnNames_ = (options.columnHeaders == ColumnHeaders::YES);
	const size_t numColumns = firstLine.size() > layout.firstField_ ? firstLine.size() - layout.firstField_ : 0;
	for (size_t column = 0; column < numColumns; ++column)
	{
		if (layout.hasColumnNames_)
		{
			layout.names_.emplace_back(firstLine[layout.firstField_ + column]);
			//As rapidcsv's GetColumnIdx: a repeated name means its last column.
			layout.df_.names_[layout.names_.back()] = column;
		}
		else
			layout.names_.push_back(std::to_string(column));
		layout.df_.type_.push_back(DataType::STRING);
		layout.df_.columns_.push_back(DataColumn());
	}
	return layout;
}

//The position of the column named `name`; throws if there are no column
//headers or no such column.
size_t columnIndex(const DataFrameLayout& layout, const std::string& name)
{
	if (!layout.hasColumnNames_)
		throw std::runtime_error("CSV column '" + name + "' requested by name, but the CSV has no column headers");
	const auto it = layout.df_.names_.find(name);
	if (it == layout.df_.names_.end())
		throw std::runtime_error("CSV column '" + name + "' not found");
	return it->second;
}

//Throws if `column` is past the last column.
void checkColumnIndex(const DataFrameLayout& layout, size_t column)
{
	if (column >= layout.df_.columns_.size())
		throw std::runtime_error("CSV column " + std::to_string(column) + " requested, but the CSV has " +
								 std::to_string(layout.df_.columns_.size()) + " columns");
}

//The columns to fill and their types: every column as text without a request,
//else the requested ones (none for an empty request). A column requested twice
//is filled once, with the type of its first request.
std::vector<size_t> selectColumns(DataFrameLayout& layout, const ColumnRequest* request)
{
	std::vector<size_t> selected;
	std::vector<bool> isSelected(layout.df_.columns_.size(), false);
	//Marks a column to be filled with values of `type`, unless it already is.
	auto select = [&](size_t column, DataType type)
	{
		if (isSelected[column])
			return;
		isSelected[column] = true;
		layout.df_.type_[column] = type;
		selected.push_back(column);
	};

	if (request == nullptr)
	{
		for (size_t column = 0; column < layout.df_.columns_.size(); ++column)
			select(column, DataType::STRING);
	}
	else if (std::holds_alternative<ColumnRequest::RequestsByName>(request->requests_))
	{
		for (const auto& req: std::get<ColumnRequest::RequestsByName>(request->requests_))
			select(columnIndex(layout, req.first), req.second);
	}
	else if (std::holds_alternative<ColumnRequest::RequestsByPosition>(request->requests_))
	{
		for (const auto& req: std::get<ColumnRequest::RequestsByPosition>(request->requests_))
		{
			checkColumnIndex(layout, req.first);
			select(req.first, req.second);
		}
	}
	return selected;
}

//Creates the vector of each selected column in the dataframe, and returns where
//to put each line's values. The dataframe's columns aren't resized afterwards,
//so the pointers stay valid.
std::vector<FilledColumn> createColumns(DataFrameLayout& layout, const std::vector<size_t>& selected)
{
	std::vector<FilledColumn> filled;
	for (size_t column: selected)
	{
		FilledColumn fc;
		fc.field_ = layout.firstField_ + column;
		fc.name_ = layout.names_[column];
		const DataType type = layout.df_.type_[column];
		DataColumn& data = layout.df_.columns_[column];
		if (type == DataType::DOUBLE)
		{
			data = DoubleColumn();
			fc.doubles_ = &std::get<DoubleColumn>(data);
		}
		else if (type == DataType::INTEGER)
		{
			data = IntegerColumn();
			fc.integers_ = &std::get<IntegerColumn>(data);
		}
		else
		{
			data = StringColumn();
			fc.strings_ = &std::get<StringColumn>(data);
		}
		filled.push_back(std::move(fc));
	}
	return filled;
}

//Adds the values of one line to the filled columns. Every line must have as
//many fields as the first one, though some may be empty: a line with a
//missing or extra field (a trailing separator too) is an error, not a line
//with its columns shifted.
void addLine(const std::vector<FilledColumn>& filled, std::span<const std::string_view> fields, size_t numFields, size_t lineNumber)
{
	if (fields.size() != numFields)
		throw std::runtime_error("CSV line " + std::to_string(lineNumber) + " has " + std::to_string(fields.size()) +
								 " fields, but the first line has " + std::to_string(numFields));
	for (const FilledColumn& column: filled)
	{
		const std::string_view text = fields[column.field_];
		if (column.doubles_ != nullptr)
			column.doubles_->push_back(toDouble(column, text, lineNumber));
		else if (column.integers_ != nullptr)
			column.integers_->push_back(toInteger(column, text, lineNumber));
		else
			column.strings_->emplace_back(text);
	}
}

//Reads the whole input into a dataframe: the columns of `request`, or every
//column as text without a request.
DataFrame readDataFrame(std::istream& input, const CSVOptions& options, const ColumnRequest* request)
{
	if (!input)
		throw std::runtime_error("couldn't read the CSV input: the stream is in an error state");

	CSVLineSplitter splitter(input, options.separator);
	//Without a first line there are no columns, so a requested column is
	//reported missing.
	const bool hasFirstLine = splitter.nextLine();
	DataFrameLayout layout = layoutDataFrame(splitter.fields(), options);
	const std::vector<size_t> selected = selectColumns(layout, request);
	if (!hasFirstLine)
		return std::move(layout.df_);
	const std::vector<FilledColumn> filled = createColumns(layout, selected);

	const size_t numFields = splitter.fields().size();
	//Without column headers, the first line is already data.
	if (!layout.hasColumnNames_)
		addLine(filled, splitter.fields(), numFields, splitter.lineNumber());
	while (splitter.nextLine())
		addLine(filled, splitter.fields(), numFields, splitter.lineNumber());

	return std::move(layout.df_);
}

//As readDataFrame, from a file.
DataFrame readDataFrameFile(const std::string& filename, const CSVOptions& options, const ColumnRequest* request)
{
	std::ifstream input(filename);
	if (!input)
		throw std::runtime_error("couldn't open CSV file '" + filename + "'");
	//The errors name the line and the column; this adds the file.
	try
	{
		return readDataFrame(input, options, request);
	}
	catch (const std::runtime_error& error)
	{
		throw std::runtime_error("file '" + filename + "': " + error.what());
	}
}

//A request of the columns named `names`, read as text.
ColumnRequest textColumns(const std::vector<std::string>& names)
{
	ColumnRequest::RequestsByName requests;
	for (const std::string& name: names)
		requests.emplace_back(name, DataType::STRING);
	return ColumnRequest(requests);
}

} // namespace


//The following functions provide the overloads that allow different types of
//usages: retrieve all columns, only some, some typed columns, etc.

//Every column of the text, as text.
DataFrame dataFrameFromCSVString(const std::string& csvContents, TU::CSVOptions options)
{
	std::istringstream contents_stream(csvContents);
	return readDataFrame(contents_stream, options, nullptr);
}

//The requested columns of the text, each with its type.
DataFrame dataFrameFromCSVString(const std::string& csvContents, const ColumnRequest& requestedColumns, TU::CSVOptions options)
{
	std::istringstream contents_stream(csvContents);
	return readDataFrame(contents_stream, options, &requestedColumns);
}

//The columns of the text with the given names, as text.
DataFrame dataFrameFromCSVString(const std::string& csvContents, const std::vector<std::string>& requestedColumns, TU::CSVOptions options)
{
	const ColumnRequest request = textColumns(requestedColumns);
	std::istringstream contents_stream(csvContents);
	return readDataFrame(contents_stream, options, &request);
}

//Every column of the file, as text.
DataFrame dataFrameFromCSVFile(const std::string& filename, TU::CSVOptions options)
{
	return readDataFrameFile(filename, options, nullptr);
}

//The columns of the file with the given names, as text.
DataFrame dataFrameFromCSVFile(const std::string& filename, const std::vector<std::string>& requestedColumns, TU::CSVOptions options)
{
	const ColumnRequest request = textColumns(requestedColumns);
	return readDataFrameFile(filename, options, &request);
}

//The requested columns of the file, each with its type.
DataFrame dataFrameFromCSVFile(const std::string& filename, const ColumnRequest& requestedColumns, TU::CSVOptions options)
{
	return readDataFrameFile(filename, options, &requestedColumns);
}

}
