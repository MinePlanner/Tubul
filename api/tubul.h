//
// Created by Nicolas Loira on 11-01-23.
//

#pragma once

#include <vector>
#include <optional>
#include <stdexcept>
#ifndef TUBUL_MACOS
#include <source_location>
#endif
#include <string>
#include <string_view>

#include <optional>
#include <tubul_defs.h>

namespace TU {

    // setup
    void init();

    int getVersion();

	/** Simple range iterators.
	 * The idea is that you can use the irange functions to iterate over a range
	 * containing (0, N-1) just the same way you would write a for loop. For example
	 * a vector V with N elements can be iterated by
	 * 					for (auto i: TU::irange(V.size())
	 * The irange with just 1 param, assumes you start from 0, it's the same as using
	 * TU::irange(0, V.size(), but there are too many cases for when you want to iterate
	 * a slice of things in a container.
	 * The irange option with the "step" parameter is equivalent to iterate doing it+=step
	 * on each iteration, which can be useful for example to go over the odd/even numbers
	 * in a range, looping over multiples or what not. Be careful that the begin-end range
	 * does NOT need to be an exact multiple of step for ease of use (like iterate multiples
	 * of 3 between 0 and 10 would be TU::irange(0,11,3)), but that could prove a bit confusing
	 * for some particular use cases.
	 */
	tubul_range irange(size_t end);
	tubul_range irange(size_t begin, size_t end);
	tubul_skip_range irange(size_t begin, size_t end, size_t step);

	/** String handling functions.
	 * Split will return a vector of string_views with the tokens detected. Do note
	 * there are 2 versions. If you don't pass the argument, the delimiter is assumed to be
	 * a space and also consecutive spaces are considered one. When the delimiter is given
	 * the consecutive delimiters are considered as if they are surrounding an empty string. This
	 * can be very important when working with csv files.
	 *
	 * Join is the opposite to split, where you can pass the delimiters and a set of strings
	 * to be merged, and returns the new resulting string. Do note you can pass iterators if
	 * you don't want to merge a whole container.
	 */
	std::vector< std::string_view > split(std::string const& input);
	std::vector< std::string_view > split(std::string const& input, std::string const& delims);
    std::vector<std::string_view> split(std::string_view const &input);
    std::vector<std::string_view> split(std::string_view const &input, std::string const &delims);

    template<typename ContainerType>
    std::string join(ContainerType const &container, std::string const &joiner);

    template<typename IteratorType>
    std::string join(IteratorType begin, IteratorType end, std::string const &joiner);

	// logger
#ifdef TUBUL_MACOS
	[[nodiscard]] std::runtime_error throwError(const std::string &msg, int line = __builtin_LINE(),
												const char *file = __builtin_FILE(),
												const char *function = __builtin_FUNCTION());
#else
	[[nodiscard]] std::runtime_error throwError(const std::string &message,
												const std::source_location location =
													std::source_location::current());
#endif

	/** Argument parsing facilities.
	 * You can parse arguments command line arguments with the functions on this section.
	 * For adding arguments, you can use addArgument to set up a given argument
	 * Example:
	 * 		TU::addArgument("-f", "--fruit")
	 * 				.defaultValue("apple");
	 * 		TU::addArgument("-a", "--allowance")
	 * 				.setAsDouble()
	 * 				.required();
	 * 		TU::addArgument("-i", "--invitees")
	 * 				.setAsList();
	 * 		TU::addArgument("-v", "--verbose)
	 * 				.flag();
	 *
	 * 	With parseArgsOrDie you call the argument handler to do its magic.
	 * 	Then you can use getArg to retrieve the value of a given argument (be consistent with
	 * 	expected values and default types!) or query the existence of a given argument with isArgPresent
	 * 	which returns an optional of the adequate type.
	 * 	Be mindful that if you expect something as integer or double, you have to set it
	 * 	explicitly while configuring the argument. You can also set something to expect
	 * 	a list of strings with SetAsList (it will expect at least one value!), in which case the
	 * 	expected return is a vector of strings.
	 */


	struct Argument;

	void parseArgsOrDie(int argc, char** argv);
	Argument addArgument(std::string const& short_form, std::string const& long_form);

	template<typename T>
	T getArg(std::string const& param);

	template <typename T>
	std::optional<T> getOptionalArg( std::string const& param);

	/** Time measuring facilities.
	 * This are very simple objects that try to provide direct utility.
	 * The stopwatches are objects used to measure time in the scope they
	 * are created. AutoStopwatch is created with a message and when it
	 * goes out of scope, will print that message along with the time
	 * elapsed since its creation until the destruction of the object.
	 * StopWatch works similarly, but instead simply adds the elapsed
	 * time into a variable for later logging.
	 * Timer is created with a given duration and can tell you if that
	 * duration already expired.
	 */
	struct AutoStopWatch;
	struct StopWatch;
	struct Timer;

	/** The following 2 functions are mostly to ease the access to the
	 * most common operations of substracting 2 timestamps. You can use:
	 *
	 * TimePoint begin = TU::now();
	 * doStuff(); //this function takes time.
	 * std::cout << "doStuff lasted "<< TU::getDifference(begin) << "s".
	 *
	 * Basically the function now gets the current time, and the getDifference
	 * function can calculate how many seconds have passed since the time passed
	 * as parameter. It simply calls now() inside and calculates the difference
	 * (but in seconds, even if it's calculated with high accuracy). Also you can
	 * pass 2 TimePoints if you want to get the time between 2 timestamps
	 */
	double getDifference(TimePoint tp);
	double getDifference(TimePoint tp_begin, TimePoint tp_end);


	/** Processing blocks
	 * The idea of the process blocks is that you can have some extra logging
	 * and/or information regarding what your application is doing at certain
	 * point because you explicitly define some areas that represent a certain
	 * process.
	 * For this, you can use the object "ProcessBlock" passing a name of the block
	 * and it will mark the beginning of a block that will live until that
	 * process block goes out of scope. Then you can retrieve the current location
	 * using getCurrentBlockLocation to get a kind-of breadcrumbs description
	 *
	 * Example
	 * void bar(){
	 * 		TU::ProcessBlock p("bar");
	 * 		//This line should write something like "main() > inside foo > bar"
	 * 		std::cout << getCurrentBlockLocation() << std::endl;
	 * 	}
	 * void foo() {
	 * 		TU::ProcessBlock p("Inside foo");
	 * 		bar();
	 * }
	 * int main(){
	 * 		TU::ProcessBlock ("main()");
	 * 		foo();
	 * }
	 *
	 */
	struct ProcessBlock;
	std::string getCurrentBlockLocation();

	/** Tubul Exception
	 * Starting point for error reporting from Tubul. TU::Exception inherits
	 * from std::runtime_error, and will be catched similarly by a catch (std::runtime_error& e)
	 * but also supports adding extra information via operator << so eventually
	 * you can call catch (TU::Exception& e) { std::cout << e.to_string();} to
	 * get all the information stored through the different catch points during stack
	 * unwinding.
	 */
	struct Exception;


	/** CSV Handling/Dataframes
	 * The DataFrames are the main way that we expect to use CSV files. The expectation
	 * is that normally the CSV files will have data meant to be used in statistics
	 * or some type of aggregation, so the main use case would be to use all the data
	 * of a given column. DataFrames can store the columns in vectors that you can
	 * query normally.
	 * You can use dataFrameFromCSVFile(<filename>) to directly get back a CSV file
	 * organized by columns in a dataframe, but this is the simpler method and will
	 * retrieve all columns and store them as strings.
	 * By using the other variants of the function that receive either a name list
	 * or a ColumnRequest object, you can choose the columns that will be populated
	 * in the dataframe (the rest is dropped) and even choose the types of the columns
	 * (for now just int/string/double).
	 * For example
	 * 		TU::ColumnRequest req({
				{"B",TU::DataType::DOUBLE},
				{"C",TU::DataType::STRING},
				});
	   		auto df = TU::dataFrameFromCSVFile( "file.csv", req);
	   		TU::DoubleColumn dcol = std::get<TU::DoubleColumn>( df["B"]);
	 * This will populate the columns "B" and "C" in df with the contents of file.csv and
	 * then you can retrieve the columns with the correct type using std::get<COLUMN_TYPE>.
 	 * The CSVColumn object stores the columns as variants of either strings
	 * or doubles so when you retrieve a given column, you have to use std::get of
	 * the appropriate type. You can use operator[] on the columns object to retrieve
	 * a column by name or index.
	 * You can further customize some details of the parsing by selecting if the csv
	 * file has headers for columns/rows and the separator character.
	 *
	 * To read a csv file and just do basic operations, you use TU::readCsv("some_filename.csv")
	 * and the file is read and parsed to memory closely to what is written in the file. You
	 * can query some simple things like number of rows/cols, retrieve a given row or a column.
	 * You can request the columns as vectors of strings, integers or doubles by choosing the
	 * function:
	 * std::vector<double> getColumnAsDouble(size_t colIndex) const;
	 * std::vector<long> getColumnAsInteger(size_t colIndex) const;
	 * std::vector<std::string> getColumnAsString(size_t colIndex) const;
	 *
	 *

	 */

	struct CSVContents;
	struct DataFrame;

	std::optional<CSVContents> readCsv(std::string const& filename );
	std::optional<CSVContents> readCsvFromString(std::string const& contents );
	DataFrame dataFrameFromCSVString(const std::string& csvContents, CSVOptions options );
	DataFrame dataFrameFromCSVString(const std::string& csvContents, const std::vector<std::string>& requestedColumns, CSVOptions options );
	DataFrame dataFrameFromCSVString(const std::string& csvContents, const ColumnRequest& requestedColumns, CSVOptions options );
	DataFrame dataFrameFromCSVFile(const std::string& filename, CSVOptions );
	DataFrame dataFrameFromCSVFile(const std::string& filename, const std::vector<std::string>& requestedColumns, CSVOptions options );
	DataFrame dataFrameFromCSVFile(const std::string& filename, const ColumnRequest& requestedColumns, CSVOptions options );

	std::string memCurrentRSS();
	std::string memPeakRSS();
}