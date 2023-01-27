//
// Created by Carlos Acosta on 27-01-23.
//

#include <chrono>
#include <iostream>
#include "tubul_time.h"

namespace TU
{




AutoStopWatch::AutoStopWatch(const std::string& msg):
	start_(std::chrono::high_resolution_clock::now()),
	msg_(msg)
{	}

AutoStopWatch::~AutoStopWatch()
{
	auto end_time = std::chrono::high_resolution_clock::now();
	TimeDuration elapsed = (end_time-start_);
	std::cout << msg_ << elapsed.count() << std::endl;
}

double AutoStopWatch::elapsed()
{
	auto end_time = std::chrono::high_resolution_clock::now();
	TimeDuration elapsed = (end_time-start_);
	return elapsed.count();
}


StopWatch::StopWatch(TimeDuration& td):
	start_(std::chrono::high_resolution_clock::now()),
	out_(td)
{	}

StopWatch::~StopWatch()
{
	auto end_time = std::chrono::high_resolution_clock::now();
	TimeDuration elapsed = (end_time-start_);
	out_ += elapsed;
}

double StopWatch::elapsed()
{
	auto end_time = std::chrono::high_resolution_clock::now();
	TimeDuration elapsed = (end_time-start_);
	return elapsed.count();
}



Timer::Timer(const TimeDuration& td):
	start_(std::chrono::high_resolution_clock::now()),
	wait_time_(td)
{}

double Timer::remaining()
{
	auto current = std::chrono::high_resolution_clock::now();
	TimeDuration elapsed = (current-start_);
	auto remaining = (wait_time_ - elapsed).count();
	if (remaining>0)
		return remaining;
	else return 0.0;
}
bool Timer::alive()
{
	auto current = std::chrono::high_resolution_clock::now();
	TimeDuration elapsed = (current-start_);
	return (wait_time_ - elapsed).count() > 0;
}
}