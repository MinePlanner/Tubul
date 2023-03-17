//
// Created by Carlos Acosta on 02-02-23.
//

#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <utility>

namespace TU
{

using DoubleColumn = std::vector<double>;
using IntegerColumn = std::vector<long>;
using StringColumn = std::vector<std::string>;
using DataColumn = std::variant<std::monostate, DoubleColumn, IntegerColumn, StringColumn>;

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
	CSVContents(CSVContents&& other) noexcept ;
	~CSVContents();

	CSVContents& operator=(CSVContents&& other) noexcept;

	size_t rowCount() const;
	size_t colCount() const;
	std::vector<std::string> getRow(size_t rowIndex) const;
	std::vector<std::string> getColNames() const;
	std::vector<double> getColumnAsDouble(size_t colIndex) const;
	std::vector<long> getColumnAsInteger(size_t colIndex) const;
	std::vector<std::string> getColumnAsString(size_t colIndex) const;

	DataFrame convertAllToColumnFormat();
	DataFrame convertToColumnFormat(std::vector<std::string> const& columns);
	DataFrame convertToColumnFormat(std::vector<size_t> const& columns);

	std::unique_ptr<CSVRawData> impl_;
};


}