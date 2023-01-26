//
// Created by Nicolas Loira on 1/26/23.
//

#include "tubul.h"
#include <gtest/gtest.h>

TEST(TUBULLogger, testLogError)
{
	EXPECT_THROW(
		{
			try
			{
				throw TU::logError("test");
			}
			catch (const std::runtime_error &e)
			{
				EXPECT_EQ("Error: 'test' at function TestBody", std::string(e.what()).substr(0, 34));
				throw;
			}
		},
		std::runtime_error);
}