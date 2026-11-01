//
// Created by Carlos Acosta on 09-02-23.
//

#include <gtest/gtest.h>

#include <vector>
#include <cstdio>
#include <fstream>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "tubul.h"


const char* CSV1 = R"(name,A,B,C,D
hugo,1,2,3,4
paco,4,3,2,1
luis,3,1,2,4
)";

const char* CSV2 = R"(name;A;B;C;D
hugo;1;2;3;4
paco;4;3;2;1
luis;3;1;2;4
)";

const TU::StringColumn expectedColNames = {"hugo","paco","luis"};
const TU::StringColumn expectedColAs = {"1","4","3"};
const TU::StringColumn expectedColBs = {"2","3","1"};
const TU::StringColumn expectedColCs = {"3","2","2"};
const TU::StringColumn expectedColDs = {"4","1","4"};

const TU::DoubleColumn expectedColAd = {1,4,3};
const TU::DoubleColumn expectedColBd = {2,3,1};
const TU::DoubleColumn expectedColCd = {3,2,2};
const TU::DoubleColumn expectedColDd = {4,1,4};

template <typename TubulDataColumnType>
void testColumn(const TubulDataColumnType& col, const TubulDataColumnType& expected )
{
  EXPECT_EQ(col.size(), expected.size());
  bool colEqual = std::equal(col.begin(), col.end(), expected.begin() );
  if (!colEqual)
	  std::cout << "Break here" << std::endl;
  EXPECT_EQ(colEqual, true);
};

//CSV1 compressed with gzip (Python: gzip.compress(CSV1, mtime=0)).
const unsigned char CSV1_GZIP[] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xcb, 0x4b, 0xcc, 0x4d, 0xd5, 0x71,
	0xd4, 0x71, 0xd2, 0x71, 0xd6, 0x71, 0xe1, 0xca, 0x28, 0x4d, 0xcf, 0xd7, 0x31, 0xd4, 0x31, 0xd2,
	0x31, 0xd6, 0x31, 0xe1, 0x2a, 0x48, 0x4c, 0xce, 0xd7, 0x31, 0x01, 0x32, 0x8d, 0x74, 0x0c, 0xb9,
	0x72, 0x4a, 0x33, 0x8b, 0x81, 0x4c, 0x90, 0x9c, 0x09, 0x17, 0x00, 0x7c, 0x5c, 0x1a, 0x35, 0x34,
	0x00, 0x00, 0x00
};

//Writes `size` bytes to a new file named `filename`.
void writeBytes(std::string_view filename, const unsigned char* bytes, size_t size)
{
	std::ofstream out(std::string(filename), std::ios::binary);
	out.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(size));
}

TEST(TUBULCSV, testBasicFunctionality)
{
	std::vector<size_t> expected   = {0, 1, 2, 3, 4};
	size_t              iter_count = 0;
	auto res = TU::readCsvFromString(CSV1);
	EXPECT_TRUE(res);
	auto const& csv_data = res.value();
	//General data peeking
	EXPECT_EQ( csv_data.colCount(),4);
	EXPECT_EQ( csv_data.rowCount(),3);

	//Checking the name of the columns
	auto const& col_names = csv_data.getColNames();
	std::vector<std::string> expected_col_names = {"A","B","C","D"};
	EXPECT_EQ(col_names.size(), expected_col_names.size());
	for (size_t i = 0; i < col_names.size(); ++i)
		EXPECT_EQ(col_names[i], expected_col_names[i]);

	//Getting a given row (the first row)
	auto hugo = csv_data.getRow(0);
	std::vector<std::string> expected_hugo = {"1","2","3","4"};
	EXPECT_EQ(hugo.size(), expected_hugo.size());
	for (size_t i = 0; i < hugo.size(); ++i )
		EXPECT_EQ(expected_hugo[i], hugo[i]);

	//Getting a given row (the last row)
	auto luis = csv_data.getRow(2);
	std::vector<std::string> expected_luis = {"3","1","2","4"};
	EXPECT_EQ(luis.size(), expected_luis.size());
	for (size_t i = 0; i < hugo.size(); ++i )
		EXPECT_EQ(expected_hugo[i], hugo[i]);

	//Get the second column (column B)
	auto colB = csv_data.getColumnAsInteger(1);
	std::vector<int> expected_colB= {2,3,1};
	EXPECT_EQ(colB.size(), expected_colB.size());
	for (size_t i = 0; i < colB.size(); ++i )
		EXPECT_EQ(expected_colB[i], colB[i]);

	//Getting all rows
	std::vector<std::vector<std::string>> expected_rows = {
		{"1","2","3","4"},
		{"4","3","2","1"},
		{"3","1","2","4"}
	};
	size_t idx = 0;
	for(auto const &row : csv_data.rows())
		EXPECT_EQ(row, expected_rows[idx++]);
	//Get column index by name
	std::optional <int> goodIndex = 1;
	std::string goodTest = "B";
	EXPECT_EQ(csv_data.getColumnIndex(goodTest), 1);
	//Get nonexisting column
	std::optional <int> badIndex = -1;
	std::string badTest = "Z";
	EXPECT_EQ(csv_data.getColumnIndex(badTest), std::nullopt);
}

TEST(TUBULCSV, testDataframeString)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 );

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colA = std::get<TU::StringColumn>(df["A"]);
	testColumn(colA, expectedColAs);

	auto colB = std::get<TU::StringColumn>(df["B"]);
	testColumn( colB, expectedColBs );

	auto colC = std::get<TU::StringColumn>(df["C"]);
	testColumn( colC, expectedColCs );

	auto colD = std::get<TU::StringColumn>(df["D"]);
	testColumn( colD, expectedColDs );
}

TEST(TUBULCSV, testDataframeStringColumnsByName1)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 ,{"B","C"});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colB = std::get<TU::StringColumn>(df["B"]);
	testColumn( colB, expectedColBs);

	auto colC = std::get<TU::StringColumn>(df["C"]);
	testColumn( colC, expectedColCs );

	//These columns shouldn't have data!
	EXPECT_ANY_THROW( auto colA = std::get<TU::StringColumn>(df["A"]) );
	EXPECT_ANY_THROW( auto colD = std::get<TU::StringColumn>(df["D"]) );
}

TEST(TUBULCSV, testDataframeStringColumnsByName2)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 ,{"A","C"});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colA = std::get<TU::StringColumn>(df["A"]);
	testColumn( colA, expectedColAs);

	auto colC = std::get<TU::StringColumn>(df["C"]);
	testColumn( colC, expectedColCs );

	//These columns shouldn't have data!
	EXPECT_ANY_THROW( auto colB = std::get<TU::StringColumn>(df["B"]) );
	EXPECT_ANY_THROW( auto colD = std::get<TU::StringColumn>(df["D"]) );
}

TEST(TUBULCSV, testDataframeStringColumnsByName3)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 ,{"B","D"});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colB = std::get<TU::StringColumn>(df["B"]);
	testColumn( colB, expectedColBs);

	auto colD = std::get<TU::StringColumn>(df["D"]);
	testColumn( colD, expectedColDs );

	//These columns shouldn't have data!
	EXPECT_ANY_THROW( auto colA = std::get<TU::StringColumn>(df["A"]) );
	EXPECT_ANY_THROW( auto colC = std::get<TU::StringColumn>(df["C"]) );
}

TEST(TUBULCSV, testDataframeStringColumnsByName4)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 ,{"C","A"});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colA = std::get<TU::StringColumn>(df["A"]);
	testColumn( colA, expectedColAs);

	auto colC = std::get<TU::StringColumn>(df["C"]);
	testColumn( colC, expectedColCs );

	//These columns shouldn't have data!
	EXPECT_ANY_THROW( auto colB = std::get<TU::StringColumn>(df["B"]) );
	EXPECT_ANY_THROW( auto colD = std::get<TU::StringColumn>(df["D"]) );
}

TEST(TUBULCSV, testDataframeColumnsByNameAndType1)
{
	TU::ColumnRequest req({
		{"A",TU::DataType::DOUBLE},
		{"B",TU::DataType::DOUBLE},
		{"C",TU::DataType::DOUBLE},
		{"D",TU::DataType::DOUBLE}
	});
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 , req);

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colA = std::get<TU::DoubleColumn>(df["A"]);
	testColumn( colA, expectedColAd );
	auto colB = std::get<TU::DoubleColumn>(df["B"]);
	testColumn( colB, expectedColBd );
	auto colC = std::get<TU::DoubleColumn>(df["C"]);
	testColumn( colC, expectedColCd );
	auto colD = std::get<TU::DoubleColumn>(df["D"]);
	testColumn( colD, expectedColDd );
	auto colA2 = std::get<TU::DoubleColumn>(df[0]);
	testColumn( colA2, expectedColAd );
	auto colB2 = std::get<TU::DoubleColumn>(df[1]);
	testColumn( colB2, expectedColBd );
	auto colC2 = std::get<TU::DoubleColumn>(df[2]);
	testColumn( colC2, expectedColCd );
	auto colD2 = std::get<TU::DoubleColumn>(df[3]);
	testColumn( colD2, expectedColDd );

	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["B"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["C"]) );
}

TEST(TUBULCSV, testDataframeColumnsByNameAndType2)
{
	TU::ColumnRequest req({
							  {"A",TU::DataType::DOUBLE},
							  {"D",TU::DataType::DOUBLE}
						  });
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 , req);

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colA = std::get<TU::DoubleColumn>(df["A"]);
	testColumn( colA, expectedColAd );
	auto colD = std::get<TU::DoubleColumn>(df["D"]);
	testColumn( colD, expectedColDd );
	auto colA2 = std::get<TU::DoubleColumn>(df[0]);
	testColumn( colA2, expectedColAd );
	auto colD2 = std::get<TU::DoubleColumn>(df[3]);
	testColumn( colD2, expectedColDd );

	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["A"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["B"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["C"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["D"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::DoubleColumn>(df["B"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::DoubleColumn>(df["C"]) );
}

TEST(TUBULCSV, testDataframeColumnsByNameAndType3)
{
	TU::ColumnRequest req({
							  {"B",TU::DataType::DOUBLE},
							  {"D",TU::DataType::STRING}
						  });
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1 , req);

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 4);

	auto colB = std::get<TU::DoubleColumn>(df["B"]);
	testColumn( colB, expectedColBd );
	auto colD = std::get<TU::StringColumn>(df["D"]);
	testColumn( colD, expectedColDs );
	auto colB2 = std::get<TU::DoubleColumn>(df[1]);
	testColumn( colB2, expectedColBd );
	auto colD2 = std::get<TU::StringColumn>(df[3]);
	testColumn( colD2, expectedColDs );

	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["A"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["B"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["C"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::DoubleColumn>(df["A"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::DoubleColumn>(df["C"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::DoubleColumn>(df["D"]) );
}

TEST(TUBULCSV, testDataframeNoRowHeader)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 5);


	auto colA = std::get<TU::StringColumn>(df["A"]);
	auto colB = std::get<TU::StringColumn>(df["B"]);
	auto colC = std::get<TU::StringColumn>(df["C"]);
	auto colD = std::get<TU::StringColumn>(df["D"]);
	auto colName = std::get<TU::StringColumn>(df["name"]);

	testColumn(colA, expectedColAs);
	testColumn( colB, expectedColBs );
	testColumn( colC, expectedColCs );
	testColumn( colD, expectedColDs );
	testColumn(colName, expectedColNames);
}

TEST(TUBULCSV, testDataframeNoRowHeader2)
{
	TU::ColumnRequest req({
							  {"B",TU::DataType::DOUBLE},
							  {"C",TU::DataType::STRING},
							  {"name",TU::DataType::STRING}
						  });
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1,
                                                  req,
												   { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 5);

	auto colB = std::get<TU::DoubleColumn>(df["B"]);
	auto colC = std::get<TU::StringColumn>(df["C"]);
	auto colName = std::get<TU::StringColumn>(df["name"]);

	testColumn( colB, expectedColBd );
	testColumn( colC, expectedColCs );
	testColumn(colName, expectedColNames);
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["A"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["D"]) );
}

TEST(TUBULCSV, testDataframeNoRowHeader3)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1,
												   {"name","D"},
												   { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 5);

	auto colD = std::get<TU::StringColumn>(df["D"]);
	auto colName = std::get<TU::StringColumn>(df["name"]);

	testColumn( colD, expectedColDs );
	testColumn(colName, expectedColNames);
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["A"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["B"]) );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["C"]) );
}

TEST(TUBULCSV, testDataframeSeparator)
{
	//This case should have 1 column because it's a proper csv
	TU::DataFrame df_base = TU::dataFrameFromCSVString( CSV1, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ';'});
	auto expectedWrongCount = df_base.getColCount();
	EXPECT_EQ(expectedWrongCount, 1);

	//This case should have 1 column, because it's separated by ;, si read
	//normally should have 1 column.
	TU::DataFrame df_base2 = TU::dataFrameFromCSVString( CSV2, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});
	auto expectedWrongCount2 = df_base2.getColCount();
	EXPECT_EQ(expectedWrongCount, 1);

	//CSV2 should be separated by ";" instead of commas, but now should be
	//parsed correctly.
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV2, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ';'});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 5);


	auto colA = std::get<TU::StringColumn>(df["A"]);
	auto colB = std::get<TU::StringColumn>(df["B"]);
	auto colC = std::get<TU::StringColumn>(df["C"]);
	auto colD = std::get<TU::StringColumn>(df["D"]);
	auto colName = std::get<TU::StringColumn>(df["name"]);

	testColumn(colA, expectedColAs);
	testColumn( colB, expectedColBs );
	testColumn( colC, expectedColCs );
	testColumn( colD, expectedColDs );
	testColumn(colName, expectedColNames);
}

TEST(TUBULCSV, testDataframeNoColumnHeader)
{
	{
		//This case should have 1 column because it's a proper csv
		TU::DataFrame df_base            = TU::dataFrameFromCSVString(CSV1, {TU::ColumnHeaders::YES, TU::RowHeaders::NO, ';'});
		auto          expectedWrongCount = df_base.getColCount();
		EXPECT_EQ(expectedWrongCount, 1);
	}

	{
		// This case should have 1 column, because it's separated by ;, so read
		// normally with "," as separator should have 0 or 1 column depending on
		// row headers enabled or not.
		TU::DataFrame df_base2            = TU::dataFrameFromCSVString(CSV2 );
		auto          expectedWrongCount2 = df_base2.getColCount();
		EXPECT_EQ(expectedWrongCount2, 0);
		TU::DataFrame df_base3            = TU::dataFrameFromCSVString(CSV2, {TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});
		auto          expectedWrongCount3 = df_base3.getColCount();
		EXPECT_EQ(expectedWrongCount3, 1);
	}

	//CSV2 should be separated by ";" instead of commas, but now should be
	//parsed correctly.
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV2, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ';'});

	auto colCount = df.getColCount();
	EXPECT_EQ(colCount, 5);


	auto colA = std::get<TU::StringColumn>(df["A"]);
	auto colB = std::get<TU::StringColumn>(df["B"]);
	auto colC = std::get<TU::StringColumn>(df["C"]);
	auto colD = std::get<TU::StringColumn>(df["D"]);
	auto colName = std::get<TU::StringColumn>(df["name"]);

	testColumn(colA, expectedColAs);
	testColumn( colB, expectedColBs );
	testColumn( colC, expectedColCs );
	testColumn( colD, expectedColDs );
	testColumn(colName, expectedColNames);
}

//Streaming reader: typed and positional requests, files, CSVs without column
//headers, then corner cases and errors. Splitting lines into fields is tested
//in test_csv_line_splitter.cpp.

TEST(TUBULCSV, testDataframeIntegerColumns)
{
	TU::ColumnRequest req({
							  {"A",TU::DataType::INTEGER},
							  {"D",TU::DataType::INTEGER}
						  });
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1, req);

	testColumn( std::get<TU::IntegerColumn>(df["A"]), TU::IntegerColumn{1,4,3} );
	testColumn( std::get<TU::IntegerColumn>(df["D"]), TU::IntegerColumn{4,1,4} );
	EXPECT_EQ(df.getRowCount(), 3);
}

TEST(TUBULCSV, testDataframeColumnsByPosition)
{
	TU::ColumnRequest req({
							  {size_t(1),TU::DataType::DOUBLE},
							  {size_t(3),TU::DataType::STRING}
						  });
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1, req);

	EXPECT_EQ(df.getColCount(), 4);
	testColumn( std::get<TU::DoubleColumn>(df["B"]), expectedColBd );
	testColumn( std::get<TU::StringColumn>(df["D"]), expectedColDs );
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["A"]) );
}

TEST(TUBULCSV, testDataframeFromFile)
{
	const std::string filename = "tubul_test_csv_from_file.csv";
	{
		std::ofstream out(filename);
		out << CSV1;
	}
	TU::DataFrame all = TU::dataFrameFromCSVFile( filename, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});
	TU::DataFrame some = TU::dataFrameFromCSVFile( filename, std::vector<std::string>{"name"}, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});
	//An error in a file names the file.
	std::string message;
	try { TU::dataFrameFromCSVFile( filename, std::vector<std::string>{"Z"}, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','}); }
	catch (const std::exception& error) { message = error.what(); }
	std::remove(filename.c_str());

	EXPECT_EQ(all.getColCount(), 5);
	testColumn( std::get<TU::StringColumn>(all["C"]), expectedColCs );
	testColumn( std::get<TU::StringColumn>(some["name"]), expectedColNames );
	EXPECT_NE(message.find(filename), std::string::npos) << message;
}

TEST(TUBULCSV, testDataframeFromGzipFile)
{
	//Compressed files are recognized by their content, whatever their name.
	const std::string filename = "tubul_test_csv_from_gzip.csv.gz";
	writeBytes(filename, CSV1_GZIP, sizeof(CSV1_GZIP));
	TU::ColumnRequest req({ {"B",TU::DataType::DOUBLE}, {"name",TU::DataType::STRING} });
	TU::DataFrame df = TU::dataFrameFromCSVFile( filename, req, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});
	auto csv = TU::readCsv(filename);
	std::remove(filename.c_str());

	testColumn( std::get<TU::DoubleColumn>(df["B"]), expectedColBd );
	testColumn( std::get<TU::StringColumn>(df["name"]), expectedColNames );
	ASSERT_TRUE(csv);
	EXPECT_EQ(csv->rowCount(), 3);
}

TEST(TUBULCSV, testDataframeNoColumnHeaderByPosition)
{
	const char* noHeader = "hugo,1,2\npaco,4,3\nluis,3,1\n";
	TU::ColumnRequest req({
							  {size_t(0),TU::DataType::STRING},
							  {size_t(2),TU::DataType::DOUBLE}
						  });
	TU::DataFrame df = TU::dataFrameFromCSVString( noHeader, req, { TU::ColumnHeaders::NO, TU::RowHeaders::NO, ','});

	EXPECT_EQ(df.getColCount(), 3);
	EXPECT_EQ(df.getRowCount(), 3);
	testColumn( std::get<TU::StringColumn>(df[0]), expectedColNames );
	testColumn( std::get<TU::DoubleColumn>(df[2]), TU::DoubleColumn{2,3,1} );

	TU::DataFrame all = TU::dataFrameFromCSVString( noHeader, { TU::ColumnHeaders::NO, TU::RowHeaders::NO, ','});
	EXPECT_EQ(all.getColCount(), 3);
	testColumn( std::get<TU::StringColumn>(all[1]), TU::StringColumn{"1","4","3"} );
}

TEST(TUBULCSV, testDataframeEmptyFields)
{
	//Empty fields count: every line has as many fields as the header.
	TU::DataFrame df = TU::dataFrameFromCSVString( "name,A,B\nhugo,,2\n,1,\n", { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});
	EXPECT_EQ(std::get<TU::StringColumn>(df["name"]), (TU::StringColumn{"hugo", ""}));
	EXPECT_EQ(std::get<TU::StringColumn>(df["A"]), (TU::StringColumn{"", "1"}));
	EXPECT_EQ(std::get<TU::StringColumn>(df["B"]), (TU::StringColumn{"2", ""}));
}

TEST(TUBULCSV, testDataframeEmptyInput)
{
	TU::DataFrame df = TU::dataFrameFromCSVString( "" );
	EXPECT_EQ(df.getColCount(), 0);
	EXPECT_EQ(df.getRowCount(), 0);
	//It has no columns, so requested ones are missing.
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "", std::vector<std::string>{"A"}) );
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "\n\n", TU::ColumnRequest(TU::ColumnRequest::RequestsByPosition{ {size_t(0),TU::DataType::STRING} })) );
}

TEST(TUBULCSV, testDataframeHeaderOnly)
{
	TU::ColumnRequest req({ {"A",TU::DataType::DOUBLE} });
	TU::DataFrame df = TU::dataFrameFromCSVString( "name,A,B\n", req);
	EXPECT_EQ(df.getColCount(), 2);
	EXPECT_EQ(std::get<TU::DoubleColumn>(df["A"]).size(), 0);
}

TEST(TUBULCSV, testDataframeEmptyRequest)
{
	//An empty request fills no column, as an empty list of names does.
	TU::ColumnRequest none(TU::ColumnRequest::RequestsByName{});
	TU::DataFrame df = TU::dataFrameFromCSVString( CSV1, none);
	EXPECT_EQ(df.getColCount(), 4);
	EXPECT_ANY_THROW( auto fail = std::get<TU::StringColumn>(df["A"]) );
}

TEST(TUBULCSV, testDataframeRepeatedColumnName)
{
	//As rapidcsv: a repeated name means its last column.
	TU::DataFrame df = TU::dataFrameFromCSVString( "x,v,v\n1,2,3\n", std::vector<std::string>{"v"}, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','});
	testColumn( std::get<TU::StringColumn>(df["v"]), TU::StringColumn{"3"} );
}

TEST(TUBULCSV, testDataframeErrors)
{
	const TU::CSVOptions noRowHeaders { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','};

	//A column that isn't in the header.
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( CSV1, std::vector<std::string>{"Z"}) );
	//A column by name in a CSV without column headers.
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( CSV1, std::vector<std::string>{"A"}, { TU::ColumnHeaders::NO, TU::RowHeaders::NO, ','}) );
	//A column position past the last column.
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( CSV1, TU::ColumnRequest({ {size_t(4),TU::DataType::STRING} })) );
	//Lines with fewer or more fields than the first line, even when the
	//requested columns are there, and a trailing separator (an extra field).
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "name,A,B\nhugo,1\n", std::vector<std::string>{"A"}, noRowHeaders) );
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "name,A\nhugo,1,2\n", std::vector<std::string>{"A"}, noRowHeaders) );
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "name,A\nhugo,1,\n", std::vector<std::string>{"A"}, noRowHeaders) );
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "hugo,1\npaco\n", std::vector<std::string>{}, { TU::ColumnHeaders::NO, TU::RowHeaders::NO, ','}) );
	//Text where a number is expected.
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "name,A\nhugo,x\n", TU::ColumnRequest({ {"A",TU::DataType::DOUBLE} }), noRowHeaders) );
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "name,A\nhugo,\n", TU::ColumnRequest({ {"A",TU::DataType::INTEGER} }), noRowHeaders) );
	//A sign after '+', which std::stol rejects too.
	EXPECT_ANY_THROW( TU::dataFrameFromCSVString( "name,A\nhugo,+-5\n", TU::ColumnRequest({ {"A",TU::DataType::INTEGER} }), noRowHeaders) );
	//A file that doesn't exist.
	EXPECT_ANY_THROW( TU::dataFrameFromCSVFile( "tubul_test_csv_no_such_file.csv", noRowHeaders) );
}

TEST(TUBULCSV, testDataframeTruncatedGzipFile)
{
	//A gzip file cut short is an error, not a shorter dataframe.
	const std::string filename = "tubul_test_csv_truncated.csv.gz";
	writeBytes(filename, CSV1_GZIP, sizeof(CSV1_GZIP) - 10);
	EXPECT_ANY_THROW( TU::dataFrameFromCSVFile( filename, { TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','}) );
	EXPECT_FALSE( TU::readCsv(filename) );
	std::remove(filename.c_str());
}
