//
// Created by Nicolas Loira on 11-01-23.
//

#include <iostream>
#include <vector>
#include <thread>

#include "tubul.h"

const char* CSVSample = R"(Name,HEX,R,G,B
White,#FFFFFF,100,100,100
Silver,#C0C0C0,75,75,75
Gray,#808080,50,50,50
Black,#000000,0,0,0
Red,#FF0000,100,0,0
Maroon,#800000,50,0,0
Yellow,#FFFF00,100,100,0
Olive,#808000,50,50,0
Lime,#00FF00,0,100,0
Green,#008000,0,50,0
Aqua,#00FFFF,0,100,100
Teal,#008080,0,50,50
Blue,#0000FF,0,0,100
Navy,#000080,0,0,50
Fuchsia,#FF00FF,100,0,100
Purple,#800080,50,0,50)";

std::string getTubulMem()
{
	return std::string("Memory (current/max): ") + TU::memCurrentRSS() + " / " + TU::memPeakRSS() ;
}

void parseArguments(int argc, const char** argv)
{
	TU::addArgument("-c", "--chanchito")
		.help("Testing a flag argument")
		.setAsFlag();
	TU::addArgument( "-p", "--perrito")
		.help("su nombre de perro favorito")
		.defaultValue(std::string("cachupin"));

	TU::addArgument( "-x", "--xerox")
		.help("un numerito")
		.setAsDouble();

	TU::addArgument( "-b", "--barks")
		.help("ladridos")
		.setAsList();

	TU::parseArgsOrDie(argc, argv);
	bool askedFor = TU::getArg<bool>("-c");
	if (askedFor)
		TU::logReport() << " > I was asked for a piggy";
	else
		TU::logReport() << " > No piggy??";

	auto cachupin = TU::getArg<std::string>("-p");
	TU::logReport() <<" > The doggy i was asked is " << cachupin;

	auto magicNumber = TU::getOptionalArg<double>("-x");
	if (magicNumber)
		TU::logReport() << " I got a magic number!"  << *magicNumber;

	auto barkTypes = TU::getOptionalArg<std::vector<std::string>>("-b");
	if (barkTypes )
		TU::logReport() << "Barks: " << TU::join( *barkTypes, ",");


}

void exampleLogging()
{
	// the following will go to stdout, because no logger has been defined
	TU::logReport("Starting logger tests");

	try
	{
		throw TU::throwError("This is an example of throw error.");
	}
	catch (std::runtime_error &e)
	{
		TU::logInfo("throwError working ok, with text:");
		TU::logInfo(e.what());
	}

	// from now on, logReport and up will go to screen and
	// logInfo and up will go to a file example1.log
	TU::addLoggerDefinition(std::cout, TU::LogLevel::REPORT, TU::LogOptions::NOTIMESTAMP);
	TU::addLoggerDefinition("example1.log", TU::LogLevel::INFO);

	TU::logReport("This message should go to screen and example1.log");
	TU::logInfo("This message should go only to example1.log");
    TU::logWarning("Everybody should see this warning.");

	// without arguments, log* will behave like a stream
	// TU::logWarning() << "Everybody should see this warning." << "Everybody!";
    TU::logReport() << "There are " << 4 << " logger streams";

    TU::logDebug("Only print this message on Debug builds.");

}

void exampleStrings()
{
	std::string hello("Hello world 1 2 3");
	auto tokens = TU::split(hello );
	TU::logReport() << "I can split and join strings: " << hello << " -> '" << TU::join(tokens,"->") << "'";
	std::string_view hello_view  = hello;
	auto tokens_from_view = TU::split(hello_view);
	TU::logReport() << "Also works with string_views: " << hello_view << " -> '" << TU::join(tokens_from_view,"->") << "'";
}

void exampleRangeAndJoin()
{
	TU::logReport() << "With Tubul I can easily iterate simple ranges";
	std::vector<std::string> numbers;
	for (auto i: TU::irange(1,7))
		numbers.push_back(std::to_string(i));
	TU::logReport() << TU::join(numbers, "->");
}

void exampleTimers(TU::Timer &alarm3s)
{
	TU::logReport() <<"\tTimer: Is the alarm up?" << ((alarm3s.alive())?"YES":"NO") << "  remaining: " << alarm3s.remaining();

	TU::TimeDuration exampleElapsed;
	{
		TU::StopWatch st(exampleElapsed);
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
	TU::logReport() << "I slept for " << exampleElapsed.count() << " seconds";
	TU::logReport() <<"\tTuner: Is the alarm up?" << ( (alarm3s.alive())?"YES":"NO" ) << "  remaining: " << alarm3s.remaining();
}

int main(int argc, const char** argv){

	// the main program should create a Tubul.
	// Sub-libraries are free to use this one!
	std::cout << "Hello Tubul version: " << TU::getVersion() << ".\n";

    // start by setting up loggers
    exampleLogging();

	TU::Block b("exampleApp");
	TU::AutoStopWatch exampleTimer("Example app elapsed:");

	//Cool trick to use "3s" instead of std::chrono::seconds(3)
	using namespace std::chrono_literals;
	TU::Timer alarm3s(3s);
	{
		TU::Block parsing("Parsing");
		TU::AutoStopWatch t(std::string("Tubul example timer for parse arguments:"));
		parseArguments(argc, argv);
        TU::logReport() << TU::getCurrentBlockLocation();
	}

    TU::logReport() << "I can check some arguments! explicit and default values! (use -h for help, or -c for a flag and -p for a name)";

	exampleStrings();
	exampleRangeAndJoin();
	exampleTimers(alarm3s);

    TU::logReport()  <<"\tTimer: Is the alarm up?" << ((alarm3s.alive())?"YES":"NO") << "  remaining: " << alarm3s.remaining();

	TU::TimeDuration exampleElapsed;
	{
		TU::Block ps("sleeping");
		TU::StopWatch st(exampleElapsed);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		TU::logReport() << TU::getCurrentBlockLocation();
	}
	try
	{
		TU::logReport() << "I slept for " << exampleElapsed.count() << " seconds";
		TU::logReport() << "\tTimer: Is the alarm up?" << ((alarm3s.alive()) ? "YES" : "NO") << "  remaining: " << alarm3s.remaining();
		TU::logReport() << TU::getCurrentBlockLocation();
		throw TU::Exception("a nefarious error")  << "And it can receive more danger!" << TU::getCurrentBlockLocation();
	}
	catch (TU::Exception& r)
	{
		TU::logReport() << "catched a new exception" << r.to_string();
		TU::logReport() << "And the what also works: '" << r.what() << "'" ;

	}
	TU::logReport() << "I slept for " << exampleElapsed.count() << " seconds";
	TU::logReport() <<"\tTuner: Is the alarm up?" << ( (alarm3s.alive())?"YES":"NO" ) << "  remaining: " << alarm3s.remaining();
	TU::logReport() << TU::getCurrentBlockLocation();

	TU::logReport() << "With Tubul I can easily read CSV files (Mem before reading csv:" << getTubulMem() << ")";
	std::optional<TU::CSVContents> csv_reading_result;
	{
		TU::AutoStopWatch st("Time reading CSV file: ");
		//Here I request tubul to read it. I will get an optional if something
		//fails (file not found or parsing failure).
		auto res = TU::readCsvFromString(CSVSample) ;
		if (!res)
			TU::logReport() << "Couldn't read sample file ";
		else
			csv_reading_result.swap(res);

	}

	TU::logReport() << "Mem after reading csv:" << getTubulMem();
	{
		TU::AutoStopWatch st("Time converting some csv data to columns : ");
		auto &csv_file = *csv_reading_result;
		//With the csv file already read, I can do quick peeking at things.
		TU::logReport() << "Rows detected: " << csv_file.rowCount();
		TU::logReport() << "Columns detected: " << csv_file.colCount();

		auto white = csv_file.getRow(0);
		TU::logReport() <<" I can ask specific rows: White has values: (" << TU::join(white,",") << ")";

		//I can ask for specific columns, even if some are repeated, no extra work is done if possible.
		//I know this is an integer column.
		auto colR_int = csv_file.getColumnAsInteger(1);

		//And calculate the mean of this column.
		double mean = 0;
		for (auto x: colR_int)
			mean += x;
		TU::logReport() << "The mean of the R col is: " << (mean/colR_int.size());

		TU::logReport() << "Mem after requesting some data columns:" << getTubulMem();
	}

	//Clearing the cached column data (at this point it still exists!!)
	csv_reading_result.reset();
	//But now we want to treate all as column based data, i.e. a dataframe, with
	//the RGB columns as int, and hex as string.
	{
		using TU::DataType;
		TU::AutoStopWatch st("Time converting all data of csv to columns: ");
		TU::ColumnRequest req({ {"HEX", DataType::STRING},
							   {"R", DataType::INTEGER},
							   {"G", DataType::INTEGER},
							   {"B", DataType::INTEGER}});
		auto df = TU::dataFrameFromCSVString(CSVSample, req);
		TU::logReport() << "Rows detected: " << df.getRowCount();
		TU::logReport() << "Columns detected: " << df.getColCount();

		//Request all data columns to be prepared.
		TU::logReport() << "Mem after requesting ALL data columns:" << getTubulMem();
	}

	TU::logReport() << "Current RSS: " << TU::memCurrentRSS();
	TU::logReport() << "PeakRSS: " << TU::memPeakRSS();

}
