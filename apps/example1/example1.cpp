//
// Created by Nicolas Loira on 11-01-23.
//

#include <iostream>
#include <vector>

#include "tubul.h"

int error_function(){
	throw TU::throwError("Hay algo mal aqui");
}

void parseArguments(int argc, char** argv)
{
	TU::addArgument("-c", "--chanchito")
		.help("Testing a flag argument")
		.flag();
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

	auto magicNumber = TU::isArgPresent<double>("-x");
	if (magicNumber)
	std::cout << " I got a magic number!"  << *magicNumber << std::endl;

	auto barkTypes = TU::isArgPresent<std::vector<std::string>>("-b");
	if (barkTypes )
		std::cout << "Barks: " << TU::join( *barkTypes, ",") << std::endl ;


}

int main(int argc, char** argv){
	std::cout << "Hello Tubul version: " << TU::getVersion() << ".\n";

	parseArguments(argc, argv);

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


	// uncomment to test error location funcionality
	// error_function();
}
