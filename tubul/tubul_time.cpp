//
// Created by Carlos Acosta on 27-01-23.
//

#include <chrono>
#include <iostream>
#include "tubul_time.h"
#include "tubul_logger.h"

namespace TU
{


double elapsed(TimePoint tp)
{
	TimeDuration elapsed = (now() - tp);
	return elapsed.count();
}

double elapsed(TimePoint tp_begin, TimePoint tp_end)
{
	TimeDuration elapsed = (tp_end - tp_begin);
	return elapsed.count();
}


AutoStopWatch::AutoStopWatch(const std::string& msg):
	start_(now()),
	msg_(msg)
{	}

AutoStopWatch::~AutoStopWatch()
{
	auto end_time = now();
	TimeDuration elapsed = (end_time-start_);
	TU::logInfo() << msg_ << elapsed.count();
}

double AutoStopWatch::elapsed()
{
	auto end_time = now();
	TimeDuration elapsed = (end_time-start_);
	return elapsed.count();
}


StopWatch::StopWatch(TimeDuration& td):
	start_(now()),
	out_(td)
{	}

StopWatch::~StopWatch()
{
	auto end_time = now();
	TimeDuration elapsed = (end_time-start_);
	out_ += elapsed;
}

double StopWatch::elapsed()
{
	auto end_time = now();
	TimeDuration elapsed = (end_time-start_);
	return elapsed.count();
}



Timer::Timer(const TimeDuration& td):
	start_(now()),
	wait_time_(td)
{}

Timer::Timer(int secs):
	start_(now()),
	wait_time_(std::chrono::seconds(secs) )
{}

double Timer::remaining()
{
	auto current = now();
	TimeDuration elapsed = (current-start_);
	auto remaining = (wait_time_ - elapsed).count();
	if (remaining>0)
		return remaining;
	else return 0.0;
}
bool Timer::alive()
{
	auto current = now();
	TimeDuration elapsed = (current-start_);
	return (wait_time_ - elapsed).count() > 0;
}
}