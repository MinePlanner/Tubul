//
// Created by Nicolas Loira on 11-01-23.
//

#include <iostream>
#include <vector>
#include <thread>

#include "tubul.h"

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

}

void exampleStrings()
{
	std::string hello("Hello world 1 2 3");
	auto tokens = TU::split(hello );
	std::cout << "I can split and join strings: " << hello << " -> '" << TU::join(tokens,"->") << "'" << std::endl;
	std::string_view hello_view  = hello;
	auto tokens_from_view = TU::split(hello_view);
	std::cout << "Also works with string_views: " << hello_view << " -> '" << TU::join(tokens_from_view,"->") << "'" << std::endl;
}

void exampleRangeAndJoin()
{
	std::cout << "With Tubul I can easily iterate simple ranges\n";
	std::__1::vector<std::string> numbers;
	for (auto i: TU::irange(1,7))
		numbers.push_back(std::to_string(i));
	std::cout << TU::join(numbers, "->") << std::endl;
}

void exampleTimers(TU::Timer &alarm3s)
{
	std::cout <<"\tTimer: Is the alarm up?" << ((alarm3s.alive())?"YES":"NO") << "  remaining: " << alarm3s.remaining() << std::endl;

	TU::TimeDuration exampleElapsed;
	{
		TU::StopWatch st(exampleElapsed);
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
	std::cout << "I slept for " << exampleElapsed.count() << " seconds" << std::endl;
	std::cout <<"\tTuner: Is the alarm up?" << ( (alarm3s.alive())?"YES":"NO" ) << "  remaining: " << alarm3s.remaining() << std::endl;
}

int main(int argc, char** argv){

	// the main program should create a Tubul.
	// Sub-libraries are free to use this one!
	TU::Tubul tubul;
	std::cout << "Hello Tubul version: " << TU::getVersion() << ".\n";
	TU::AutoStopWatch exampleTimer("Example app elapsed:");

	//Cool trick to use "3s" instead of std::chrono::seconds(3)
	using namespace std::chrono_literals;
	TU::Timer alarm3s(3s);
	{
		TU::AutoStopWatch t(std::string("Tubul example timer for parse arguments:"));
		parseArguments(argc, argv);
	}

	std::cout << "I can check some arguments! explicit and default values! (use -h for help, or -c for a flag and -p for a name)" << std::endl;

	exampleStrings();
	exampleRangeAndJoin();
	exampleLogging();
	exampleTimers(alarm3s);

}
