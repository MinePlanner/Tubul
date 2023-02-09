//
// Created by Carlos Acosta on 09-02-23.
//

#include <gtest/gtest.h>
#include "tubul.h"

//Function with the only purpose of throwing an exception.
void throw_excep()
{
	throw TU::Exception("This is a tubul exception");
}
void rethrow_excep()
{
	try { throw_excep();}
	catch (TU::Exception& e)
	{
		e << "Second message!";
		throw;
	}
}

TEST(TUBULException, basic)
{
	EXPECT_THROW(throw_excep(), TU::Exception);
	EXPECT_THROW(rethrow_excep(), TU::Exception);
}

TEST(TUBULException, messages1)
{
	//Tubul exceptions should be able to store messages as they
	//are travelling through the stack.
	try
	{
		rethrow_excep();
	}
	catch (TU::Exception &e)
	{
		auto mesg = e.to_string();
		EXPECT_TRUE(mesg.find("tubul exception") != std::string::npos);
		EXPECT_TRUE(mesg.find("Second") != std::string::npos);
	}
}

TEST(TUBULException, messages2)
{
	//TU::Exceptions can be catched as a normal std::exception, but the
	//what() is only the original message (unless we learn how to solve tht).
	try{ throw_excep();}
	catch (std::exception& e)
	{
		std::string mesg = e.what();
		std::cout << "Exception as basic exception: " << mesg << std::endl;
		EXPECT_TRUE(mesg.find("tubul exception") != std::string::npos);
		EXPECT_TRUE(mesg.find("second") == std::string::npos);
	}
}
