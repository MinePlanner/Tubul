//
// Splitting the lines of a CSV stream into fields.
//

#pragma once
#include <cstddef>
#include <istream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace TU
{

/** Reads a CSV stream one line at a time and splits each line into fields,
 * following the rules of rapidcsv (used by readCsv):
 * - a field that starts with the quote character (") runs to its closing
 *   quote, so it may hold separators. The quotes around it are removed, and
 *   a doubled quote inside becomes a single one.
 * - every '\r' is dropped, so "\r\n" line endings work. This includes a
 *   '\r' inside a quoted field.
 * - a quoted field can't hold a line break: each line is a whole record, and
 *   a quote left open runs to the end of its line.
 * - a UTF-8 byte order mark at the start of the stream is dropped.
 * - lines that are empty or hold only whitespace (as TU::trim defines it:
 *   spaces, tabs, form feeds and vertical tabs) are skipped.
 * Only one line is held in memory at a time.
 */
class CSVLineSplitter
{
public:
	/** Reads `input`, with fields separated by `separator`. The stream must
	 * outlive the splitter. */
	CSVLineSplitter(std::istream& input, char separator);

	//The fields are views of the line held here, so a copy (or a move) would
	//point into another object's line.
	CSVLineSplitter(const CSVLineSplitter&) = delete;
	CSVLineSplitter& operator=(const CSVLineSplitter&) = delete;

	/** Reads the next line that isn't skipped and splits it. Returns false at
	 * the end of the input. */
	bool nextLine();

	/** The fields of the line last read. They stay valid until the next call
	 * to nextLine(). */
	std::span<const std::string_view> fields() const;

	/** The number of the line last read, counting from 1 and including the
	 * skipped lines. */
	size_t lineNumber() const;

private:
	std::istream& input_;
	const char separator_;
	std::string line_;
	size_t lineNumber_ = 0;
	std::vector<std::string_view> fields_;
};

}
