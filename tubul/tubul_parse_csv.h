//
// Created by Carlos Acosta on 02-02-23.
//

#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <utility>
#include "tubul_types.h"

namespace TU
{

using DoubleColumn = std::vector<double>;
using IntegerColumn = std::vector<int64_t>;
using StringColumn = std::vector<std::string>;
using DataColumn = std::variant<std::monostate, DoubleColumn, IntegerColumn, StringColumn>;

/** The columns to read from a CSV, by name or by position, each with the type
 * of its values. A field must be wholly a value of its column's type, or the
 * read throws an error naming the line, the column and the text; whitespace
 * around the value and a leading '+' or '-' are allowed.
 * - DataType::INTEGER: decimal (42, -7, and 0012 reads as 12), hexadecimal
 *   with a 0x prefix (0x1F), or octal with a 0o prefix (0o17), in the range of
 *   int64_t. A leading 0 alone doesn't mean octal, as it does in C.
 * - DataType::DOUBLE: decimal (2.5, -1e-3, inf, nan), or hexadecimal with a
 *   0x prefix (0x1.8p3, as printf's %a writes it).
 * - DataType::STRING: any text, as it is in the file.
 */
struct ColumnRequest
{
	using PositionType = std::pair<size_t, TU::DataType>;
	using NameType = std::pair<std::string, TU::DataType>;
	using RequestsByPosition = std::vector<PositionType>;
	using RequestsByName = std::vector<NameType>;

	ColumnRequest(const RequestsByPosition& req):
		requests_(req){};
	ColumnRequest(const RequestsByName& req):
		requests_(req){};

	size_t size() const;

	ColumnRequest& add(size_t, TU::DataType);
	ColumnRequest& add(const std::string&, TU::DataType);

	std::variant < std::monostate, RequestsByPosition , RequestsByName >  requests_;
};

struct DataFrame
{
	const DataColumn& operator[](size_t idx) const;
	const DataColumn& operator[](const std::string& name) const;

	size_t getColCount() const;
	size_t getRowCount() const;

	std::unordered_map<std::string, size_t> names_;
	std::vector<DataType> type_;
	std::vector<DataColumn> columns_;

};

struct CSVContents
{
	struct CSVRawData;


	explicit CSVContents(std::string const& filename);
	explicit CSVContents(std::istream& input_stream);
	explicit CSVContents( std::unique_ptr<CSVRawData>&& rawData);
	CSVContents(CSVContents&& other) noexcept ;
	~CSVContents();

	CSVContents& operator=(CSVContents&& other) noexcept;

	size_t rowCount() const;
	size_t colCount() const;
	std::vector<std::string> getRow(size_t rowIndex) const;
	std::vector<std::string> getColNames() const;
	std::vector<double> getColumnAsDouble(size_t colIndex) const;
	std::vector<int64_t> getColumnAsInteger(size_t colIndex) const;
	std::vector<std::string> getColumnAsString(size_t colIndex) const;
	auto rows() const;
	
	std::optional <size_t> getColumnIndex(const std::string_view& name) const;

	std::unique_ptr<CSVRawData> impl_;
};

inline
auto CSVContents::rows() const {
	struct iterator
	{
		size_t idx;
		const CSVContents &contents;

		bool operator != (const iterator &other) const {return other.idx != idx;}
		iterator operator ++ (){
			idx++;
			return *this;
		}
		std::vector<std::string> operator * () const {return contents.getRow(idx);}
	};
	struct iterator_wrapper
	{
		const CSVContents &contents;

		auto begin(){ return iterator{0, contents}; }
		auto end(){ return iterator{contents.rowCount(), contents}; }
	};

	return iterator_wrapper{*this};
}

struct CSVOptions
{
	ColumnHeaders columnHeaders;
	RowHeaders rowHeaders;
	char separator;
	/** How fields are separated. With SeparatorMode::WHITESPACE_RUNS, they are
	 * separated by runs of whitespace (as TU::trim defines it: spaces, tabs,
	 * form feeds and vertical tabs), and `separator` isn't used: several spaces
	 * between two fields are one separator, and whitespace at the start or end
	 * of a line is ignored (as in files written with columns aligned by
	 * padding). Quoted fields may hold whitespace. A blank cell isn't a field, so a
	 * line with one has a field missing, which is an error: write "" for an
	 * empty value. Used by the dataFrameFromCSV functions. */
	SeparatorMode separatorMode = SeparatorMode::CHARACTER;
};

DataFrame dataFrameFromCSVFile(const std::string& filename, CSVOptions options = {ColumnHeaders::YES, RowHeaders::YES, ','});
DataFrame dataFrameFromCSVFile(const std::string& filename, const std::vector<std::string>& requestedColumns, CSVOptions options = {ColumnHeaders::YES, RowHeaders::YES, ','});
DataFrame dataFrameFromCSVFile(const std::string& filename, const ColumnRequest& requestedColumns, CSVOptions options = {ColumnHeaders::YES, RowHeaders::YES, ','});
DataFrame dataFrameFromCSVString(const std::string& csvContents, CSVOptions options = {ColumnHeaders::YES, RowHeaders::YES, ','});
DataFrame dataFrameFromCSVString(const std::string& csvContents, const std::vector<std::string>& requestedColumns, CSVOptions options = {ColumnHeaders::YES, RowHeaders::YES, ','});
DataFrame dataFrameFromCSVString(const std::string& csvContents, const ColumnRequest& requestedColumns, CSVOptions options = {ColumnHeaders::YES, RowHeaders::YES, ','});
}