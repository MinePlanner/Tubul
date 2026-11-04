//
// Created by Carlos Acosta on 09-02-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <vector>


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

//CSV1's rows without its header line.
const char* CSV_NO_HEADERS = R"(hugo,1,2,3,4
paco,4,3,2,1
luis,3,1,2,4
)";

//How CSV_NO_HEADERS (and CSV1, with headers) are read.
const TU::CSVOptions NO_HEADERS =
	{TU::ColumnHeaders::NO, TU::RowHeaders::NO, ','};
const TU::CSVOptions ROW_LABELS =
	{TU::ColumnHeaders::NO, TU::RowHeaders::YES, ','};
const TU::CSVOptions HEADERS =
	{TU::ColumnHeaders::YES, TU::RowHeaders::NO, ','};

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

//A csv without column headers: its columns have no names, and are read whole
//or requested by position.
TEST(TUBULCSV, testDataframeWithoutColumnHeaders)
{
	TU::DataFrame all = TU::dataFrameFromCSVString(CSV_NO_HEADERS, NO_HEADERS);
	ASSERT_EQ(all.getColCount(), 5u);
	testColumn(std::get<TU::StringColumn>(all[0]), expectedColNames);
	testColumn(std::get<TU::StringColumn>(all[2]), expectedColBs);

	TU::ColumnRequest::RequestsByPosition positions =
		{{0, TU::DataType::STRING}, {2, TU::DataType::DOUBLE}};
	TU::DataFrame some = TU::dataFrameFromCSVString(CSV_NO_HEADERS,
		TU::ColumnRequest(positions), NO_HEADERS);
	testColumn(std::get<TU::StringColumn>(some[0]), expectedColNames);
	testColumn(std::get<TU::DoubleColumn>(some[2]), expectedColBd);
	//A column that wasn't requested, or past the last one, can't be read.
	EXPECT_THROW(some[1], std::runtime_error);
	EXPECT_THROW(some[5], std::runtime_error);

	//With row labels, the first column holds them: positions count from the
	//next one.
	TU::DataFrame labelled = TU::dataFrameFromCSVString(CSV_NO_HEADERS,
		TU::ColumnRequest(positions), ROW_LABELS);
	ASSERT_EQ(labelled.getColCount(), 4u);
	testColumn(std::get<TU::StringColumn>(labelled[0]), expectedColAs);
	testColumn(std::get<TU::DoubleColumn>(labelled[2]), expectedColCd);

	//Integer columns count their rows too.
	TU::ColumnRequest::RequestsByPosition integers =
		{{1, TU::DataType::INTEGER}};
	TU::DataFrame integerFrame = TU::dataFrameFromCSVString(CSV_NO_HEADERS,
		TU::ColumnRequest(integers), NO_HEADERS);
	EXPECT_EQ(integerFrame.getRowCount(), 3u);
	EXPECT_EQ(std::get<TU::IntegerColumn>(integerFrame[1]),
		TU::IntegerColumn({1, 4, 3}));
}

//A file in the temp directory, under a name of its own (test runs may overlap),
//removed when the test ends, whether it passes or not.
struct TempCsvFile
{
	explicit TempCsvFile(const char* contents)
	{
		std::ofstream out(path_);
		out << contents;
		if (!out)
		{
			throw std::runtime_error(
				std::format("Couldn't write {}", path_.string()));
		}
	}
	TempCsvFile(const TempCsvFile&) = delete;
	TempCsvFile& operator=(const TempCsvFile&) = delete;
	~TempCsvFile()
	{
		std::error_code ignored;
		std::filesystem::remove(path_, ignored);
	}

	const std::filesystem::path path_ = std::filesystem::temp_directory_path()
		/ std::format("tubul_test_csv_{}.csv", std::random_device()());
};

//The same csv read from a file: whole, by typed position, and the missing
//columns.
TEST(TUBULCSV, testDataframeFileWithoutColumnHeaders)
{
	const TempCsvFile file(CSV_NO_HEADERS);
	TU::DataFrame all = TU::dataFrameFromCSVFile(file.path_.string(),
		NO_HEADERS);
	ASSERT_EQ(all.getColCount(), 5u);
	testColumn(std::get<TU::StringColumn>(all[0]), expectedColNames);
	testColumn(std::get<TU::StringColumn>(all[2]), expectedColBs);

	//By typed position.
	TU::ColumnRequest::RequestsByPosition positions =
		{{0, TU::DataType::STRING}, {2, TU::DataType::DOUBLE}};
	TU::DataFrame some = TU::dataFrameFromCSVFile(file.path_.string(),
		TU::ColumnRequest(positions), NO_HEADERS);
	testColumn(std::get<TU::StringColumn>(some[0]), expectedColNames);
	testColumn(std::get<TU::DoubleColumn>(some[2]), expectedColBd);

	//Missing columns: a position past the end, and any name.
	TU::ColumnRequest::RequestsByPosition pastTheEnd =
		{{5, TU::DataType::STRING}};
	EXPECT_THROW(TU::dataFrameFromCSVFile(file.path_.string(),
		TU::ColumnRequest(pastTheEnd), NO_HEADERS), std::runtime_error);
	TU::ColumnRequest::RequestsByName byName = {{"A", TU::DataType::DOUBLE}};
	EXPECT_THROW(TU::dataFrameFromCSVFile(file.path_.string(),
		TU::ColumnRequest(byName), NO_HEADERS), std::runtime_error);
	EXPECT_THROW(TU::dataFrameFromCSVFile(file.path_.string(),
		std::vector<std::string>{"A"}, NO_HEADERS), std::runtime_error);
}

//A request for a column the csv doesn't have throws, rather than reading or
//writing out of range: a position past the last column, or a name the csv
//lacks (every name, without headers).
TEST(TUBULCSV, testDataframeRequestsForMissingColumns)
{
	TU::ColumnRequest::RequestsByPosition pastTheEnd =
		{{5, TU::DataType::STRING}};
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV_NO_HEADERS,
		TU::ColumnRequest(pastTheEnd), NO_HEADERS), std::runtime_error);
	//With row labels the csv has one column less: position 4 is past the end.
	TU::ColumnRequest::RequestsByPosition pastTheLabelledEnd =
		{{4, TU::DataType::STRING}};
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV_NO_HEADERS,
		TU::ColumnRequest(pastTheLabelledEnd), ROW_LABELS), std::runtime_error);

	TU::ColumnRequest::RequestsByName byName = {{"A", TU::DataType::DOUBLE}};
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV_NO_HEADERS,
		TU::ColumnRequest(byName), NO_HEADERS), std::runtime_error);
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV_NO_HEADERS,
		std::vector<std::string>{"A"}, NO_HEADERS), std::runtime_error);

	TU::ColumnRequest::RequestsByName misspelled =
		{{"Z", TU::DataType::DOUBLE}};
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV1,
		TU::ColumnRequest(misspelled), HEADERS), std::runtime_error);
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV1,
		std::vector<std::string>{"Z"}, HEADERS), std::runtime_error);

	//With row labels, the header of the label column names no data column.
	const TU::CSVOptions headersAndRowLabels =
		{TU::ColumnHeaders::YES, TU::RowHeaders::YES, ','};
	TU::ColumnRequest::RequestsByName labelHeader =
		{{"name", TU::DataType::STRING}};
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV1,
		TU::ColumnRequest(labelHeader), headersAndRowLabels),
		std::runtime_error);
	EXPECT_THROW(TU::dataFrameFromCSVString(CSV1,
		std::vector<std::string>{"name"}, headersAndRowLabels),
		std::runtime_error);
}
