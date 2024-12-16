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

    //////////
    // Setup
    /////////

	/** Start Tubul engine. Setups data structures, timers, memory control, among others
	 */
    void init();

	/** Get Tubul version in format major*10000 + minor*100 + patch
	 * For example, version 1.3.4 will be reported as 10304.
	 * In this way, it's easy to chech if Tubul version is the minimun required
	 * For example: if(TU::getVersion() < 10200){ # error: Required tubul 1.2 or higher }
	 * @return Single int variation of version
	 */
    int getVersion();

	//////////
	// Utils
	/////////

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

	////////////
	// Strings
	///////////

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

    /** The slinerange function will return an object that can be iterated
     * with the range loop idiom (or using begin/end iterators if that's your
     * style). Each item of the range is a line contained in the input string
     * s (using "\n" as separator). If you load the contents of a text file into
     * memory, you can iterate very easily by using slinerange. It's very similar
     * to std::getline, but uses string_views to avoid copies and has more c++
     * flavour imho.
     *
     * for ( auto line: TU::slinerange(my_string) )
     *      std::cout << "This is a line: " << line << std::endl;
     *
     */
    StringLineRange slinerange(const std::string& s);
    StringLineRange slinerange(const std::string_view s);



    /** Commonly used functions to remove the trailing whitespace of strings
  * Do note, these only work on string views so be aware if passing strings!
  */
    inline
    std::string_view ltrim(const std::string_view s);
    inline
    std::string_view rtrim(const std::string_view s);
    inline
    std::string_view trim(const std::string_view &s);
    /** Don't use these functions!!!! These will fail and are here just to
     * ensure theres no funny business pasing temporary strings
     */
    inline std::string_view ltrim(std::string&& s);
    inline std::string_view rtrim(std::string&& s);
    inline std::string_view trim(std::string&& s);
	////////////
	// Logger
	////////////

    /** \brief throwError is a function that will log the indicated
     * message, including where (file, line) it was invoked.
     * \return A TU::Exception with the content of the message,
     * exception that should not be ignored.
     */
#ifdef TUBUL_MACOS
    [[nodiscard]] TU::Exception throwError(
            const std::string &msg, int line,
            const char *file,
            const char *function);

#else
	[[nodiscard]] TU::Exception throwError(const std::string &message,
												const std::source_location location);
#endif

    /** \brief Adds a new logger sink/target, which can be either
     * a std::ostream or the name (string) of a logger file.
     * Tubul will take care of the opened named files, when it dies.
     * @param logfile A string with the name of a file where log events
     *                  will be written.
     * @param level A level from where this sink should be used.
     *              See TU::LogLevel.
     * @param options Optional Options that can be added using "|",
     *              like adding colors, skipping timestamps, etc.
     *              See TU::LogOptions
     */
	void addLoggerDefinition(std::string const &logfile, LogLevel level, LogOptions options=LogOptions::NONE);
	void addLoggerDefinition(std::ostream &out, LogLevel level, LogOptions options=LogOptions::NONE);

    /** \brief Clears logger definitions defined previously,
     * including the default std::cout definition.
     */
    void clearLoggerDefinitions();

    /** \brief log* functions, allow to send a message to all loggers that
     * participate on the corresponding level.
     * End-of-line is added atomatically.
     * @param The message to be sent
     */
    void logError(std::string const &msg);
    void logWarning(std::string const &msg);
	void logReport(std::string const &msg);
    void logInfo(std::string const &msg);
    void logDevel(std::string const &msg);
    void logStat(std::string const &msg);

    /** \brief log* functions, allow to send a message to all loggers that
     * participate on the corresponding level Similar to normal log* versions
     * but this version is thread safe.
     * End-of-line is added atomatically.
     * @param The message to be sent
     */
    void safelogError(std::string const &msg);
    void safelogWarning(std::string const &msg);
	void safelogReport(std::string const &msg);
    void safelogInfo(std::string const &msg);
    void safelogDevel(std::string const &msg);
    void safelogStat(std::string const &msg);

    /** \brief log streams, which allow to send strings and numbers using
     * the "<<" operator. Example:
     * TU::logInfo() << "We have " << numCars << " cars ready to ship.";
     * End-of-line is added automatically.
     * @return
     */
    LogStreamU logError();
    LogStreamU logWarning();
    LogStreamU logReport();
    LogStreamU logInfo();
    LogStreamU logDevel();
    LogStreamU logStat();

    /** \brief log streams, which allow to send strings and numbers using
     * the "<<" operator. Similar to normal log*() functions but these are
     * thread-safe.  Example:
     * TU::logInfo() << "We have " << numCars << " cars ready to ship.";
     * End-of-line is added automatically.
     * @return
     */
    LogStreamTS safelogError();
    LogStreamTS safelogWarning();
    LogStreamTS safelogReport();
    LogStreamTS safelogInfo();
    LogStreamTS safelogDevel();
    LogStreamTS safelogStats();
	/////////
	// Args
	/////////

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

	void parseArgsOrDie(int argc, const char** argv);
	Argument addArgument(std::string const& short_form, std::string const& long_form);

	template<typename T>
	T getArg(std::string const& param);

	template <typename T>
	std::optional<T> getOptionalArg( std::string const& param);


    std::string getArgsHelp();

	//////////
	// Timers
	/////////

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
	double elapsed(TimePoint tp);
	double elapsed(TimePoint tp_begin, TimePoint tp_end);


    //////////
    // Blocks
    //////////


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
	struct Block;
	std::string getCurrentBlockLocation();

    /////////////
    // Exceptions
    /////////////


    /** Tubul Exception
	 * Starting point for error reporting from Tubul. TU::Exception inherits
	 * from std::runtime_error, and will be catched similarly by a catch (std::runtime_error& e)
	 * but also supports adding extra information via operator << so eventually
	 * you can call catch (TU::Exception& e) { std::cout << e.to_string();} to
	 * get all the information stored through the different catch points during stack
	 * unwinding.
	 */
	struct Exception;

    ///////
    // Data
    ///////

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

    /////////
    // Memory
    /////////

	/** Self-explanatory functions - Do note these are reported by the OS*/
    size_t memCurrentRSS();
	size_t memPeakRSS();
	std::string bytesToStr(size_t bytes);

	/** These functions track memory allocated via new/delete. Tubul tries to
	 * keep track of how many bytes have been allocated at a given time so it could
	 * be used to detect at which point a big number of allocations happened, even
	 * if the current rss does not change in the end, for example, succesively
	 * allocating and then deleting those objects. Lifetime memory is always
	 * increasing, while alive can vary over time.
	 */
	size_t memAlive();
	size_t memLifetime();

	/** This structure provides an easy way to maintain a memory monitoring system.
	 * It will create a thread that will wake up every 0.5s and write to a file the
	 * current rss and peakrss at that point. The filename is provided.
	 * The memory monitor will auto-clean after itself, meaning that as soon as the
	 * object goes out of scope, it will close the file and wait for the thread
	 * to properly exit. Do note this may cause a delay given the way the thread
	 * communicates that it has finished.
	 */
	struct MemoryMonitor;

    /////////
    // File Utils
    /////////
    /** Set of simple functions to perform tasks that are common
     * but regularly require typing a lot more than just these names
     * in order to do them. For example the strToXXX use the from_chars functions
     * which are awesome, but require a couple extra steps to be used which are
     * very cumbersome.
     */
    bool isRegularFile(const std::string_view& name);
    size_t countCharInFile( const std::string_view& filename, char c);

    double strToDouble(const std::string_view& p);
    int strToInt(const std::string_view& p);
	std::string readToString( const std::string& filename);



    /////////
    // Params
    /////////
    /** Set of functions related to read parameter files so users of the application
     * can provide certain parameters that will affect some behavior.
     */


}
