//
// Created by Carlos Acosta on 13-01-23.
//
#include <string>
#include <variant>
#include <optional>
#include "tubul_argparse.h"
#include <iostream>

namespace TU{

argparse::ArgumentParser& getArgumentsParser()
{
	static argparse::ArgumentParser program("TubulApplication");
	return program;
}

Argument::Argument(argparse::Argument& arg): arg_(arg) { }

Argument& Argument::required() { arg_.required(); return *this; }
Argument& Argument::help(std::string const& help_text) { arg_.help(help_text); return *this; }
Argument& Argument::flag(){ arg_.implicit_value(true).default_value(false); return *this;}
Argument& Argument::defaultValue( bool val ){ arg_.default_value( val); return *this;}
Argument& Argument::defaultValue( int val){  arg_.default_value( val).scan<'d',int>(); return *this;}
Argument& Argument::defaultValue( double val){  arg_.default_value( val).scan<'g',double>(); return *this;}
Argument& Argument::defaultValue( std::string const& val){  arg_.default_value( val); return *this;}



Argument addArgument(std::string const& short_form, std::string const& long_form)
{
	return Argument (getArgumentsParser().add_argument(short_form, long_form));
}

void parseArgsOrDie(int argc, char** argv)
{
	try
	{
		getArgumentsParser().parse_args(argc, argv);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << getArgumentsParser() << std::endl;
		std::exit(1);

	}
}

template<typename T>
T getArg(std::string const& param)
{
	auto value = getArgumentsParser().get<T>(param);
	return value;
}

template bool getArg<bool>(std::string const& param);
template int getArg<int>(std::string const& param);
template double getArg<double>(std::string const& param);
template std::string getArg<std::string>(std::string const& param);




template <typename T>
std::optional<T> isArgPresent( std::string const& param)
{
	auto res = getArgumentsParser().present<T>( param );
	return res;
}

template std::optional<bool> isArgPresent<bool>( std::string const& param);
template std::optional<int> isArgPresent<int>( std::string const& param);
template std::optional<double> isArgPresent<double>( std::string const& param);
template std::optional<std::string> isArgPresent<std::string>( std::string const& param);
}