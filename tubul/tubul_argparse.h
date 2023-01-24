//
// Created by Carlos Acosta on 20-01-23.
//
#pragma once
#include <argparse.hpp>


namespace TU
{


class Argument
{
public:
	using ArgImpl = argparse::Argument;
	explicit Argument(ArgImpl& a);

	Argument& required();
	Argument& help(std::string const& help_text);
	Argument& flag();
	Argument& defaultValue( bool val );
	Argument& defaultValue( int val );
	Argument& defaultValue( double val );
	Argument& defaultValue( std::string const& val );
	Argument& setAsDouble();
	Argument& setAsInteger();
	Argument& setAsList();

private:
	ArgImpl& arg_;
};

}