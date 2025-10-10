//
// Created by Carlos Acosta on 09-02-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
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



constexpr std::string_view kBlockData = R"(X,Y,Z,value
0, 0, 0, 1.
0, 0, 1, 2.
0, 0, 2, 3.
0, 1, 0, 4.
0, 1, 1, 5.
0, 1, 2, 6.
0, 2, 0, 7.
0, 2, 1, 8.
0, 2, 2, 9.
1, 0, 0, 1.1
1, 0, 1, 1.2
1, 0, 2, 1.3
1, 1, 0, 1.4
1, 1, 1, 1.5
1, 1, 2, 1.6
1, 2, 0, 1.7
1, 2, 1, 1.8
1, 2, 2, 1.9
2, 0, 0, 2.1
2, 0, 1, 2.2
2, 0, 2, 2.3
2, 1, 0, 2.4
2, 1, 1, 2.5
2, 1, 2, 2.6
2, 2, 0, 2.7
2, 2, 1, 2.8
2, 2, 2, 2.9)";

TEST(
	TUBULCSV,
	DanielCase)
{
	TU::ColumnRequest req(
			{{"X", TU::DataType::DOUBLE},
			 {"Z", TU::DataType::DOUBLE},
			 {"Value", TU::DataType::DOUBLE}});
	TU::DataFrame df = TU::dataFrameFromCSVString(
		std::string(kBlockData),req);
	const auto& colX = std::get<TU::DoubleColumn>( df["X"]);
	std::cout  << "First val: " << colX.at(0) << std::endl;
}