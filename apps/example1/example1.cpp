//
// Created by Nicolas Loira on 11-01-23.
//

#include <iostream>
#include <vector>
#include <thread>
#include <sys/resource.h>

#include "tubul.h"

int error_function(){
	throw TU::throwError("Hay algo mal aqui");
}

std::string getMemUsage()
{
	struct rusage myUsage;
	getrusage(RUSAGE_SELF, &myUsage);
	std::vector<std::string> units ={"b", "kb", "mb", "gb"};
	//This starts in bytes;
	double value = myUsage.ru_maxrss;
	for (auto const& unit: units)
	{
		if (value < 1024)
			return std::to_string(value) + unit;
		value /= 1024;
	}
	return std::to_string(value) + "gb";

}

void parseArguments(int argc, char** argv)
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
		std::cout << " > I was asked for a piggy" << std::endl;
	else
		std::cout << " > No piggy??" << std::endl;

	std::string cachupin = TU::getArg<std::string>("-p");
	std::cout <<" > The doggy i was asked is " << cachupin << std::endl;

	auto magicNumber = TU::getOptionalArg<double>("-x");
	if (magicNumber)
	std::cout << " I got a magic number!"  << *magicNumber << std::endl;

	auto barkTypes = TU::getOptionalArg<std::vector<std::string>>("-b");
	if (barkTypes )
		std::cout << "Barks: " << TU::join( *barkTypes, ",") << std::endl ;


}

int main(int argc, char** argv){
	std::cout << "Hello Tubul version: " << TU::getVersion() << ".\n";
	TU::ProcessBlock b("exampleApp");
	TU::AutoStopWatch exampleTimer("Example app elapsed:");
	//Cool trick to use "3s" instead of std::chrono::seconds(3)
	using namespace std::chrono_literals;
	TU::Timer alarm3s(3s);
	{
		TU::ProcessBlock parsing("Parsing");
		TU::AutoStopWatch t(std::string("Tubul example timer for parse arguments:"));
		parseArguments(argc, argv);
		std::cout << TU::getCurrentBlockLocation() << std::endl;
	}

	std::cout << "I can check some arguments! explicit and default values! (use -h for help, or -c for a flag and -p for a name)" << std::endl;

	std::string hello("Hello world 1 2 3");
	auto tokens = TU::split(hello ) ;
	std::cout << "I can split and join strings: " << hello << " -> '" << TU::join(tokens,"->") << "'" << std::endl;
	std::string_view hello_view  = hello;
	auto tokens_from_view = TU::split(hello_view);
	std::cout << "Also works with string_views: " << hello_view << " -> '" << TU::join(tokens_from_view,"->") << "'" << std::endl;

	std::cout << "With Tubul I can easily iterate simple ranges\n";
	std::vector<std::string> numbers;
	for (auto i: TU::irange(1,7))
		numbers.push_back(std::to_string(i));
	std::cout << TU::join(numbers, "->") << std::endl;
	std::cout <<"\tTimer: Is the alarm up?" << ((alarm3s.alive())?"YES":"NO") << "  remaining: " << alarm3s.remaining() << std::endl;

	TU::TimeDuration exampleElapsed;
	{
		TU::ProcessBlock ps("sleeping");
		TU::StopWatch st(exampleElapsed);
		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::cout << TU::getCurrentBlockLocation() << std::endl;
	}
	std::cout << "I slept for " << exampleElapsed.count() << " seconds" << std::endl;
	std::cout <<"\tTuner: Is the alarm up?" << ( (alarm3s.alive())?"YES":"NO" ) << "  remaining: " << alarm3s.remaining() << std::endl;
	std::cout << TU::getCurrentBlockLocation() << std::endl;


	std::cout << "With Tubul I can easily read CSV files (Mem before reading csv:" << getMemUsage() << ")" << std::endl;
	std::optional<TU::CSVContents> csv_reading_result;
	{
		TU::AutoStopWatch st("Time reading CSV file: ");
		std::string filename = "salvador.csv";
		//Here I request tubul to read it. I will get an optional if something
		//fails (file not found or parsing failure).
		auto res = TU::read_csv(filename) ;
		if (!res)
			std::cout << "Couldn't read file " << filename << std::endl;
		else
			csv_reading_result.swap(res);

	}

	std::cout << "Mem after reading csv:" << getMemUsage() << std::endl;
	{
		TU::AutoStopWatch st("Time converting some csv data to columns : ");
		auto &csv_file = *csv_reading_result;
		//With the csv file already read, I can do quick peeking at things.
		std::cout << "Rows detected: " << csv_file.rowCount() << std::endl;
		std::cout << "Columns detected: " << csv_file.colCount() << std::endl;

		//I can ask for specific columns, even if some are repeated, no extra work is done if possible.
		std::vector<std::string> colsToConvert = {"ALTE","CAN","CUT","CAN", "ALTE", "REC","SURVEYUG_FACTOR1_1"};
		auto cols = csv_file.convertToColumnFormat(colsToConvert);

		//I know this is an integer column.
		auto alte_col_int = std::get<TU::IntegerColumn>(cols["ALTE"]);

		//And calculate the mean of this column.
		double mean = 0;
		for (auto x: alte_col_int)
			mean += x;
		std::cout << "The mean of the ALTE col is: " << (mean/alte_col_int.size()) << std::endl;

		std::cout << "Mem after requesting some data columns:" << getMemUsage() << std::endl;
	}

	//Clearing the cached column data (at this point it should exist!!)
	{
		TU::AutoStopWatch st("Time converting all data of csv to columns: ");
		auto &csv_file = *csv_reading_result;
		std::cout << "Rows detected: " << csv_file.rowCount() << std::endl;
		std::cout << "Columns detected: " << csv_file.colCount() << std::endl;

		//Request all data columns to be prepared.
		auto cols = csv_file.convertAllToColumnFormat();
		std::cout << "Mem after requesting ALL data columns:" << getMemUsage() << std::endl;
		(void) cols;
	}

	// uncomment to test error location funcionality
	// error_function();
}
