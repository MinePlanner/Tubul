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

	CSVContents(std::string const& filename);
	CSVContents(std::istream& input_stream);
	CSVContents(CSVContents&& other);
	~CSVContents();

	size_t rowCount() const;
	size_t colCount() const;
	std::vector<std::string> getColNames() const;
	std::vector<double> getColumnAsDouble(size_t colIndex) const;
	std::vector<long> getColumnAsInteger(size_t colIndex) const;
	std::vector<std::string> getColumnAsString(size_t colIndex) const;

	std::unique_ptr<CSVImpl> impl_;
};

}