//
// Created by Carlos Acosta on 21-03-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
#include <thread>

TEST(TUBULTime, BasicTime)
{
	using namespace std::chrono_literals;
	using std::chrono::seconds;
	using std::chrono::milliseconds;

	TU::TimePoint begin = TU::now();
	TU::TimePoint end = begin+3s;
	EXPECT_EQ( 3, TU::elapsed(begin, end));
	end = begin + 5s;
	EXPECT_EQ( 5, TU::elapsed(begin, end));

	end = begin + milliseconds (5500);
	EXPECT_EQ( 5.5, TU::elapsed(begin, end));

	double spentTime = TU::elapsed(begin);
	//Not sure about the value, but I would expect the clock to be more
	//precise than a millisecond.
	EXPECT_LT(spentTime, 0.001);

	//After sleeping 1s, the time should be more than 1, but likely very little more
	std::this_thread::sleep_for(seconds(1));
	spentTime = TU::elapsed(begin);
	EXPECT_GT(spentTime, 1 );
	EXPECT_LT(spentTime, 2 );
}
