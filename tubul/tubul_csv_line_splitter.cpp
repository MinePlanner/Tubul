//
// Created by Daniel Espinoza on 01-11-26.
// Splitting the lines of a CSV stream into fields.
//

#include "tubul_csv_line_splitter.h"
#include "tubul_string.h"

namespace TU
{

namespace
{

//The character that quotes a field.
constexpr char kQuote = '"';

//The byte order mark (the character U+FEFF) as UTF-8 writes it. Some programs,
//such as Excel when saving "CSV UTF-8", put it at the start of a file to mark
//its encoding. It isn't text: kept, it would become part of the first field,
//so the first column's name wouldn't match. These three bytes aren't
//printable characters, so they can only be written as escapes.
constexpr std::string_view kUtf8ByteOrderMark = "\xEF\xBB\xBF";

//Removes a byte order mark from the start of the line, if it has one (only the
//stream's first line may).
void dropByteOrderMark(std::string& line)
{
	if (line.starts_with(kUtf8ByteOrderMark))
		line.erase(0, kUtf8ByteOrderMark.size());
}

//Removes every '\r' from the line, so a file with Windows line endings ("\r\n")
//reads as one with "\n".
void dropCarriageReturns(std::string& line)
{
	if (line.find('\r') == std::string::npos)
		return;
	size_t kept = 0;
	for (char c: line)
	{
		if (c != '\r')
			line[kept++] = c;
	}
	line.resize(kept);
}

//True if the line is empty or holds only whitespace, as TU::trim defines it
//(spaces, tabs, form feeds and vertical tabs; the line has no '\n' or '\r').
bool isBlank(std::string_view line)
{
	return ltrim(line).empty();
}

//True for a whitespace character, as TU::trim defines it.
bool isWhitespace(char c)
{
	return details::WHITESPACE.find(c) != std::string::npos;
}

//True if `c` ends a field: the separator, or whitespace with
//SeparatorMode::WHITESPACE_RUNS (nextLine then skips the rest of the run).
bool isSeparator(char c, char separator, SeparatorMode mode)
{
	if (mode == SeparatorMode::WHITESPACE_RUNS)
		return isWhitespace(c);
	return c == separator;
}

//Finds where the field that begins at `begin` ends: at the first separator
//outside quotes, or at the end of the line. As rapidcsv, quotes open and close
//quoting only in a field that starts with a quote; anywhere else, a quote is a
//plain character.
size_t findFieldEnd(std::string_view line, size_t begin, char separator, SeparatorMode mode)
{
	const bool startsWithQuote = begin < line.size() && line[begin] == kQuote;
	bool quoted = false;
	size_t i = begin;
	for (; i < line.size(); ++i)
	{
		if (line[i] == kQuote && startsWithQuote)
			quoted = !quoted;
		else if (!quoted && isSeparator(line[i], separator, mode))
			break;
	}
	return i;
}

//As rapidcsv: removes the quotes around a field that starts and ends with one,
//and makes each doubled quote inside single. The text is written over the
//field's own characters, from its start (it only gets shorter). Returns the
//length of the field's text.
size_t unquote(std::string& line, size_t begin, size_t end)
{
	if (end - begin < 2 || line[begin] != kQuote || line[end - 1] != kQuote)
		return end - begin;
	size_t kept = begin;
	for (size_t i = begin + 1; i + 1 < end; ++i)
	{
		const char c = line[i];
		line[kept++] = c;
		if (c == kQuote && i + 2 < end && line[i + 1] == kQuote)
			++i;
	}
	return kept - begin;
}

} // namespace

CSVLineSplitter::CSVLineSplitter(std::istream& input, char separator, SeparatorMode mode):
	input_(input),
	separator_(separator),
	mode_(mode)
{}

//Quoted fields are unquoted in the line itself, so every field is a view of
//the line.
bool CSVLineSplitter::nextLine()
{
	while (std::getline(input_, line_))
	{
		++lineNumber_;
		if (lineNumber_ == 1)
			dropByteOrderMark(line_);
		dropCarriageReturns(line_);
		//A line with nothing but whitespace carries no data, as an empty one:
		//both are skipped instead of read as a record of blank fields.
		if (isBlank(line_))
			continue;

		fields_.clear();
		size_t begin = 0;
		while (true)
		{
			//Runs of whitespace are one separator, and whitespace at the start
			//or end of the line is ignored.
			if (mode_ == SeparatorMode::WHITESPACE_RUNS)
			{
				while (begin < line_.size() && isWhitespace(line_[begin]))
					++begin;
				if (begin == line_.size())
					break;
			}
			const size_t end = findFieldEnd(line_, begin, separator_, mode_);
			fields_.emplace_back(line_.data() + begin, unquote(line_, begin, end));
			if (end == line_.size())
				break;
			begin = end + 1;
		}
		return true;
	}
	return false;
}

std::span<const std::string_view> CSVLineSplitter::fields() const
{
	return fields_;
}

size_t CSVLineSplitter::lineNumber() const
{
	return lineNumber_;
}

}
