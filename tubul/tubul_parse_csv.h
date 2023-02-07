//
// Created by Carlos Acosta on 02-02-23.
//

#pragma once
#include <memory>
#include <vector>
#include <string>
#include <iosfwd>

namespace TU
{


struct CSVContents
{
	struct CSVImpl;
	struct CSVColumns;


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
	void convertAllToColumnFormat();
	void convertToColumnFormat(std::vector<std::string> const& columns);
	void convertToColumnFormat(std::vector<size_t> const& columns);

	void clearCurrentColums();

	std::unique_ptr<CSVImpl> impl_;
	std::unique_ptr<CSVColumns> cols_;
};

}