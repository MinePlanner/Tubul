//
// Created by Carlos Acosta on 12-01-23.
//

#include <gtest/gtest.h>
#include <algorithm>
#include "tubul.h"

const char* test_string1 = R"(This is a test
for the line iterator
that uses
string views)";
const char* test_string2 = R"(This is a test
for the line iterator
that uses
string views
)";

TEST(TUBULString, testSplitEmpty){
	std::vector<std::string> tests = {""," ", "  " } ;
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string );
		EXPECT_EQ(result.size(), 0);
		//Because we just found delimers, there's nothing "real" to check
	}
	//Now the same, but with string views.
	std::vector<std::string_view> test_views;
	for (auto const& it: tests)
		test_views.emplace_back(it);
	for ( auto const& test_string: test_views)
	{
		auto result = TU::split(test_string );
		EXPECT_EQ(result.size(), 0);
		//Because we just found delimers, there's nothing "real" to check
	}
}

TEST(TUBULString, testSplit1){

	std::vector<std::string> tests = {"P"," P", "P  ","    P    "  } ;
	std::string expected = "P";
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string );
		EXPECT_EQ(result.size(), 1);
		EXPECT_EQ(result.front(), expected);
	}
	//Now the same, but with string views.
	std::vector<std::string_view> test_views;
	for (auto const& it: tests)
		test_views.emplace_back(it);
	for ( auto const& test_string: test_views)
	{
		auto result = TU::split(test_string );
		EXPECT_EQ(result.size(), 1);
		EXPECT_EQ(result.front(), expected);
	}
}

TEST(TUBULString, testSplit2){

	std::vector<std::string> tests = {
		"Pedrito clavo un clavito",
		"  Pedrito clavo un clavito",
		"Pedrito clavo un clavito   ",
		"     Pedrito clavo un clavito   ",
		"     Pedrito    clavo  un   clavito   "
	} ;
	std::vector<std::string> expected = {"Pedrito","clavo", "un", "clavito"};
	for ( auto const& test_string: tests)
	{
		auto result = TU::split(test_string);
		EXPECT_EQ(result.size(), expected.size());
		EXPECT_EQ(result, result);
	}

	//Now the same, but with string views.
	std::vector<std::string_view> test_views;
	for (auto const& it: tests)
		test_views.emplace_back(it);
	for ( auto const& test_string: test_views)
	{
		auto result = TU::split(test_string);
		EXPECT_EQ(result.size(), expected.size());
		EXPECT_EQ(result, result);
	}
}

TEST(TUBULString, testSplitStrictEmpty){

	std::vector<std::string_view> result;
	std::vector<std::string> tests = {"", ",", ",,", ",,," } ;
	size_t expected_size = 1;
	for ( auto const& test_string: tests)
	{
		result = TU::split(test_string,"," );
		EXPECT_EQ(result.size(), expected_size);
		for (auto const& it: result)
			EXPECT_EQ(it,"");
		expected_size++;
	}

	//Now the same, but with string views.
	std::vector<std::string_view> test_views;
	for (auto const& it: tests)
		test_views.emplace_back(it);
	expected_size = 1;
	for ( auto const& test_string: test_views)
	{
		result = TU::split(test_string,"," );
		EXPECT_EQ(result.size(), expected_size);
		for (auto const& it: result)
			EXPECT_EQ(it,"");
		expected_size++;
	}
}

TEST(TUBULString, testSplitStrictReal){
	std::string test_str = "Mafalda";
	auto result = TU::split(test_str,",");
	EXPECT_EQ(result.size(), 1);
	EXPECT_EQ(result.front(), test_str.c_str());

	//The first vector contains different sets of strings separated
	//by commas as we could find in a CSV. The second contains
	//the corresponding expected ersult
	std::vector<std::string> tests = {"Mafalda,Felipito",
									  "Mafalda,Felipito,Miguelito",
									  "Mafalda,,Miguelito",
									  "Mafalda,Felipito,",
									  ",Felipito,Miguelito",
									  "Mafalda,Felipito,Miguelito,Susanita",
									  ",Felipito,Miguelito,",
									  "Mafalda,,,Susanita",
										} ;
	std::vector<std::vector<std::string>> expected = { {"Mafalda","Felipito"},
													  {"Mafalda","Felipito","Miguelito"},
													   {"Mafalda","","Miguelito"},
													   {"Mafalda","Felipito",""},
													   {"","Felipito","Miguelito"},
													  {"Mafalda","Felipito","Miguelito","Susanita"},
													   {"","Felipito","Miguelito",""},
													   {"Mafalda","","","Susanita"},
													};
	size_t current_test = 0;
	for ( auto const& test_string: tests)
	{
		result = TU::split(test_string,"," );
		auto const& current_expected = expected[current_test];
		EXPECT_EQ(result.size(),current_expected.size()) ;
		bool was_equal = std::equal(result.begin(), result.end(), expected[current_test].begin());
		EXPECT_EQ(was_equal,true ) ;
		current_test++;
	}

	//Now the same, but with string views.
	std::vector<std::string_view> test_views;
	for (auto const& it: tests)
		test_views.emplace_back(it);
	current_test = 0;
	for ( auto const& test_string: test_views)
	{
		result = TU::split(test_string,"," );
		auto const& current_expected = expected[current_test];
		EXPECT_EQ(result.size(),current_expected.size()) ;
		bool was_equal = std::equal(result.begin(), result.end(), expected[current_test].begin());
		EXPECT_EQ(was_equal,true ) ;
		current_test++;
	}
}


TEST(TUBULString, testSplitCsv){
	{
		std::vector<std::string_view> expected = {""};
		std::vector<std::string_view> result = TU::splitCsv("");
		
		EXPECT_EQ(result, expected);

	}
	{
		std::vector<std::string_view> expected = {"\"A,1\""};
		std::vector<std::string_view> result = TU::splitCsv("\"A,1\"");

		EXPECT_EQ(result, expected);
	}
	{
		std::vector<std::string_view> expected = {"A"};
		std::vector<std::string_view> result = TU::splitCsv("A");

		EXPECT_EQ(result, expected);
	}
	{ 
	std::vector<std::string_view> expected = {"name", "surname","desc","\"city, country\""};
	std::vector<std::string_view> result = TU::splitCsv("name,surname,desc,\"city, country\"");

	int idx = 0;
	for(auto &string : result)
		EXPECT_EQ(string, expected[idx++]);
	}
}

TEST(TUBULString, testJoinEmpty) {

	std::vector<std::string> test;
	std::string res;
	// 100% empty test
	res = TU::join(test, "");
	EXPECT_EQ(res, "");

	// Single empty string
	test = {""};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "");

	// N empty string
	test = {"", "",""};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "");

	//Empty strings, but an actual joiner (like for csv)
	test = {"", "",""};
	res  = TU::join(test, ",");
	EXPECT_EQ(res, ",,");
}

TEST(TUBULString, testJoinNoJoiner)
{
	std::vector<std::string> test;
	std::string res;
	//Simple case
	test = {"A"};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "A");

	//Just concatenating because the joiner is empty
	test = {"A", "B"};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "AB");

	test = {"A", "B", "C"};
	res  = TU::join(test, "");
	EXPECT_EQ(res, "ABC");
}

TEST(TUBULString, testJoin){

	std::vector<std::string> test;
	std::string res;
	//Checking we can actually join 2 strings with comma
	test = {"A","B"};
	res = TU::join(test, ",");
	EXPECT_EQ(res, "A,B");

	//Checking we can actually join 2 strings with a random string
	test = {"A","B"};
	res = TU::join(test, "->");
	EXPECT_EQ(res, "A->B");

	test = {"A","B"};
	res = TU::join(test, ",");
	EXPECT_EQ(res, "A,B");

	test = {"A","B"};
	res = TU::join(test, "->");
	EXPECT_EQ(res, "A->B");
}
TEST(TUBULString, testjoinDifferentContainers){
	//cannot test st nor multiset since they have no order

	std::deque <std::string> dequeString {"this", "is", "a", "deque"};
	std::deque <int> dequeInt {4, 3, 2, 1};

	std::string res;

	res = TU::join(dequeString, "/");
	EXPECT_EQ(res, "this/is/a/deque");

	res = TU::join(dequeInt, ".");
	EXPECT_EQ(res, "4.3.2.1");
}

TEST(TUBULString, testjoinWithLambda){
	struct Book{
		std::string name;
		int pages;
	};

	Book b1 = {"C++ for Dummies", 100};
	Book b2 = {"Hamlet", 75};

	std::vector <Book> structVec = {b1, b2};

	auto f = [](struct Book book){
    std::ostringstream os;
    os << book.name << " " << book.pages;
		return os.str();
	};

	std::string res = TU::join(structVec, ", ", f);
	std::cout<<res<<"\n";
	EXPECT_EQ(res, "C++ for Dummies 100, Hamlet 75");

}


TEST(TUBULString, testLineIterator) {

    std::string test(test_string1);
    std::vector<std::string> expected1 = {
                "This is a test",
                "for the line iterator",
                "that uses",
                "string views"};
    auto range = TU::slinerange(test);
    auto it = range.begin();
    auto end = range.end();
    size_t expected_id = 0;
    while( it != end ) {
        EXPECT_EQ(*it, expected1.at(expected_id));
        ++it; ++expected_id;
    }
    expected_id = 0;
    for (auto line: TU::slinerange(test)) {
        EXPECT_EQ(line, expected1.at(expected_id));
        expected_id++;
    }

    std::string test2(test_string2);
    expected_id =0;
    expected1.emplace_back("");
    for (auto line: TU::slinerange(test2)) {
        EXPECT_EQ(line, expected1.at(expected_id));
        expected_id++;
    }

}

TEST(TUBULString, testTrim) {
    using TU::trim, TU::ltrim, TU::rtrim;
    std::string hello = "hello";
    std::string hello_l = "\n hello";
    //ltrim and trim should affect hell_l, but rtrim shoudn't change. For
    //things without spaces, nothing should happen
    EXPECT_EQ(ltrim(hello_l), hello);
    EXPECT_EQ(trim(hello_l), hello);
    EXPECT_EQ(rtrim(hello_l), hello_l);
    EXPECT_EQ(rtrim(hello), hello);
    EXPECT_EQ(ltrim(hello), hello);
    EXPECT_EQ(trim(hello), hello);

    //Opposite test
    std::string hello_r = "hello  \r";
    EXPECT_EQ(rtrim(hello_r), hello);
    EXPECT_EQ(trim(hello_r), hello);
    EXPECT_EQ(ltrim(hello_r), hello_r);

    //With strings requiring trim on both sides
    std::string hello_lr = " hello  \r";
    EXPECT_EQ(rtrim(hello_lr), hello_lr.substr(0,hello_lr.size()-3));
    EXPECT_EQ(ltrim(hello_lr), hello_lr.substr(1));

    //With whitespaces in the middle
    std::string hello_world = "hello world";
    std::string hello_world_lr = "\n hello world \r";
    EXPECT_EQ(trim(hello_world_lr), hello_world);
    EXPECT_EQ(rtrim(hello_world_lr), hello_world_lr.substr(0, hello_world_lr.size()-2));
    EXPECT_EQ(ltrim(hello_world_lr), hello_world_lr.substr(2));


    EXPECT_ANY_THROW(rtrim(hello.substr(0)));
    EXPECT_ANY_THROW(ltrim(hello.substr(0)));
    EXPECT_ANY_THROW(trim(hello.substr(0)));
}

TEST(TUBULString, testToLower) {
	auto doTest = []( const std::string& in, const std::string& expected) {
		auto res = TU::tolower(in);
		EXPECT_EQ(res, expected);
	};
	doTest("", "");
	doTest("a", "a");
	doTest("n", "n");
	doTest("fancy words", "fancy words");
	doTest("A", "a");
	doTest("N", "n");
	doTest("Fancy Words", "fancy words");
	doTest( "aTeSt", "atest");
	doTest( "L337 C0d3", "l337 c0d3");
}

TEST(TUBULString, testToUpper) {
	auto doTest = []( const std::string& in, const std::string& expected) {
		auto res = TU::toupper(in);
		EXPECT_EQ(res, expected);
	};


	doTest("", "");
	doTest("A", "A");
	doTest("N", "N");
	doTest("FANCY WORDS", "FANCY WORDS");
	doTest("a", "A");
	doTest("n", "N");
	doTest("Fancy Words", "FANCY WORDS");
	doTest( "aTeSt", "ATEST");
	doTest( "L337 C0d3", "L337 C0D3");
}

