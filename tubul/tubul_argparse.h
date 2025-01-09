//
// Created by Carlos Acosta on 20-01-23.
//
#pragma once
#include <memory>


namespace TU
{


class Argument
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

	Argument& setAsStringList();
	Argument& setAsIntList();
	Argument& setAsDoubleList();

	template <typename T>
	Argument& testFunc();

	
private:
	ArgImplPtr arg_;

	
};

}