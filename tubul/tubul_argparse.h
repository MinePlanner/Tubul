//
// Created by Carlos Acosta on 20-01-23.
//
#pragma once
#include <memory>
#include <string>


namespace TU
{


struct Argument
{
public:
	struct ArgImpl;
	using ArgImplPtr = std::unique_ptr<ArgImpl>;

	explicit Argument(ArgImplPtr&& impl);
	~Argument();

	Argument& required();
	Argument& help(std::string const& help_text);
	Argument& defaultValue( int val );
	Argument& defaultValue( double val );
	Argument& defaultValue( std::string const& val );
	Argument& defaultValue( const char* val );
	Argument& setAsFlag();
	Argument& setAsDouble();
	Argument& setAsInteger();
	Argument& setAsList();

private:
	ArgImplPtr arg_;
};

}
