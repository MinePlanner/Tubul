//
// Created by Daniel Espinoza on 01-11-26.
// Tests of TU::CSVLineSplitter.
//

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>
#include <string_view>

#include "tubul_csv_line_splitter.h"

namespace
{
using Lines = std::vector<std::vector<std::string>>;

//Splits all the lines of `text` with `separator`.
Lines splitAll(std::string_view text, char separator = ',')
{
	std::istringstream input{std::string(text)};
	TU::CSVLineSplitter splitter(input, separator);
	Lines lines;
	while (splitter.nextLine())
	{
		std::vector<std::string> fields;
		for (std::string_view field: splitter.fields())
			fields.emplace_back(field);
		lines.push_back(fields);
	}
	return lines;
}
}

TEST(TUBULCSVLineSplitter, testPlainFields)
{
	EXPECT_EQ(splitAll("name,A,B\nhugo,1,2\n"), (Lines{{"name","A","B"}, {"hugo","1","2"}}));
}

TEST(TUBULCSVLineSplitter, testOtherSeparator)
{
	EXPECT_EQ(splitAll("name;A\nhugo;1\n", ';'), (Lines{{"name","A"}, {"hugo","1"}}));
	EXPECT_EQ(splitAll("name;A\n", ','), (Lines{{"name;A"}}));
}

TEST(TUBULCSVLineSplitter, testLastLineWithoutNewline)
{
	EXPECT_EQ(splitAll("a,b\nc,d"), (Lines{{"a","b"}, {"c","d"}}));
}

TEST(TUBULCSVLineSplitter, testEmptyFields)
{
	EXPECT_EQ(splitAll(",a,,\n"), (Lines{{"","a","",""}}));
}

TEST(TUBULCSVLineSplitter, testQuotedFields)
{
	EXPECT_EQ(splitAll("hugo,\"one, two\",1\n"), (Lines{{"hugo","one, two","1"}}));
	EXPECT_EQ(splitAll("\"say \"\"hi\"\"\",2\n"), (Lines{{"say \"hi\"","2"}}));
	EXPECT_EQ(splitAll("\"a\"\"\",x\n"), (Lines{{"a\"","x"}}));
	EXPECT_EQ(splitAll("\"\"\"\",x\n"), (Lines{{"\"","x"}}));
	EXPECT_EQ(splitAll("\"\",x\n"), (Lines{{"","x"}}));
	EXPECT_EQ(splitAll("x,\"a,b\"\n"), (Lines{{"x","a,b"}}));
	//A separator right after a doubled quote is still inside the quotes.
	EXPECT_EQ(splitAll("\"a\"\",b\",c\n"), (Lines{{"a\",b","c"}}));
}

TEST(TUBULCSVLineSplitter, testQuoteInsideAField)
{
	//As rapidcsv: a quote that isn't at the start of a field is a plain character.
	EXPECT_EQ(splitAll("ab\"c,d\n"), (Lines{{"ab\"c","d"}}));
	//So a field with a space before its quote isn't quoted.
	EXPECT_EQ(splitAll(" \"a,b\",c\n"), (Lines{{" \"a","b\"","c"}}));
	//A field that starts with a quote but doesn't end with one keeps its quotes.
	EXPECT_EQ(splitAll("\"ab\"c,d\n"), (Lines{{"\"ab\"c","d"}}));
	//A quote that is never closed runs to the end of the line.
	EXPECT_EQ(splitAll("\"a,b\n"), (Lines{{"\"a,b"}}));
}

TEST(TUBULCSVLineSplitter, testWindowsLineEndings)
{
	EXPECT_EQ(splitAll("a,b\r\nc,d\r\n"), (Lines{{"a","b"}, {"c","d"}}));
}

TEST(TUBULCSVLineSplitter, testEmptyLinesAreSkipped)
{
	//Empty lines and lines of only whitespace.
	std::istringstream input("\na,b\n \t\r\n\nc,d\n  \n");
	TU::CSVLineSplitter splitter(input, ',');
	ASSERT_TRUE(splitter.nextLine());
	EXPECT_EQ(splitter.lineNumber(), 2);
	ASSERT_TRUE(splitter.nextLine());
	EXPECT_EQ(splitter.lineNumber(), 5);
	EXPECT_EQ(splitter.fields()[1], "d");
	EXPECT_FALSE(splitter.nextLine());
	//Spaces in a line with data are kept.
	EXPECT_EQ(splitAll(" a, \n"), (Lines{{" a"," "}}));
}

TEST(TUBULCSVLineSplitter, testByteOrderMark)
{
	//The byte order mark (U+FEFF) as UTF-8 writes it: three bytes that aren't
	//printable characters, so they can only be written as escapes.
	EXPECT_EQ(splitAll("\xEF\xBB\xBFname,A\n"), (Lines{{"name","A"}}));
}

TEST(TUBULCSVLineSplitter, testEmptyInput)
{
	EXPECT_TRUE(splitAll("").empty());
	EXPECT_TRUE(splitAll("\n\n").empty());
	EXPECT_TRUE(splitAll(" \n\t\n").empty());
	EXPECT_TRUE(splitAll("\f\v \n").empty());
}
