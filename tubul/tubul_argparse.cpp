//
// Created by Carlos Acosta on 13-01-23.
//
#include <string>
#include <variant>
#include <optional>
#include <iostream>
#include <argparse.hpp>
#include "tubul_argparse.h"

namespace TU{

/**
 * Creating global argument parser.
 * @return reference to the global argument parser for the application
 */
argparse::ArgumentParser& getArgumentsParser()
{
	static argparse::ArgumentParser program("TubulApplication", "1.0", argparse::default_arguments::help);
	return program;
}
/**
 * Simple class that contains a reference to the internal implementation of the
 * argument. Doesn't have a lot of intrincacies, it only exists to hide away the
 * argparse implementation from Tubul clients.
 */
struct Argument::ArgImpl
{
	ArgImpl(argparse::Argument& a):arg_(a){}
	argparse::Argument& get() {return arg_;}
	argparse::Argument& arg_;
};

//
// Implementation of the Argument class, which is pretty much
// forwarding everything to the internal argparse implementation.
Argument::Argument(ArgImplPtr&& arg): arg_(std::forward<ArgImplPtr>(arg) ) { }
Argument::~Argument(){ }

Argument& Argument::required() { arg_->get().required(); return *this; }
Argument& Argument::help(std::string const& help_text) { arg_->get().help(help_text); return *this; }
Argument& Argument::defaultValue( int val){  arg_->get().default_value( val).scan<'d',int>(); return *this;}
Argument& Argument::defaultValue( double val){  arg_->get().default_value( val).scan<'g',double>(); return *this;}
Argument& Argument::defaultValue( std::string const& val){  arg_->get().default_value( val); return *this;}
Argument& Argument::defaultValue( const char* val){  arg_->get().default_value( std::string(val)); return *this;}
Argument& Argument::setAsDouble( ){  arg_->get().scan<'g',double>(); return *this;}
Argument& Argument::setAsInteger( ){  arg_->get().scan<'d',int>(); return *this;}
Argument& Argument::setAsFlag(){ arg_->get().implicit_value(true).default_value(false); return *this;}
Argument& Argument::setAsList( ){  arg_->get().nargs(argparse::nargs_pattern::at_least_one); return *this;}


/**
 * This function creates an argument to be parsed and returns an
 * Argument object to continue customization of that particular object.
 * @param string with the abbreviated name for the param, for example -c
 * @param string with the full name of the param, for example --color
 * @return An argument object
 */
Argument addArgument(std::string const& short_form, std::string const& long_form)
{
	return Argument(std::make_unique<Argument::ArgImpl>(getArgumentsParser().add_argument(short_form, long_form) ));
}
/**
 * supports positional arguments, aswell as optional arguments with only 1 form.
 * @param name for the name of the argument
 */
Argument addArgument(std::string const& name){
	return Argument(std::make_unique<Argument::ArgImpl>(getArgumentsParser().add_argument(name) ));
}
/**
 * Very explicit call to parse arguments or die trying.
 */
void parseArgsOrDie(int argc, const char** argv)
{
	try
	{
		getArgumentsParser().parse_args(argc, argv);
	}
	catch (std::exception& e)
	{
		std::cerr << "\n" << e.what() << "\n" << std::endl;
		std::cerr << getArgumentsParser() << std::endl;
		std::exit(1);
	}
}

std::string getArgsHelp()
{
    std::stringstream helpStr;
    helpStr << getArgumentsParser() << std::endl;
    return helpStr.str();
}

/***
 * After calling parseOrDie, this function can retrieve the values of arguments.
 * @tparam Expected type of the argument
 * @param name of the argument (either short or full form)
 * @return Returns the parsed value of the param
 */
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
template std::vector<std::string> getArg<std::vector<std::string>>( std::string const& param);



/***
 * After calling parseOrDie, this function can retrieve the values of optional
 * arguments. It returns an optional to signal if the argument is found or not
 * and the eventual value of the argument.
 * @tparam Expected type of the argument
 * @param name of the argument (either short or full form)
 * @return Returns the parsed value of the param
 */

template <typename T>
std::optional<T> getOptionalArg( std::string const& param)
{
	auto res = getArgumentsParser().present<T>( param );
	return res;
}

template std::optional<bool> getOptionalArg<bool>( std::string const& param);
template std::optional<int> getOptionalArg<int>( std::string const& param);
template std::optional<double> getOptionalArg<double>( std::string const& param);
template std::optional<std::string> getOptionalArg<std::string>( std::string const& param);
template std::optional<std::vector<std::string>> getOptionalArg<std::vector<std::string>>( std::string const& param);
}