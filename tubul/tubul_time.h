//
// Created by Carlos Acosta on 27-01-23.
//
#pragma once

#include <string>
#include <chrono>

namespace TU
{
using TimeDuration = std::chrono::duration<double> ;
using TimePoint = std::chrono::system_clock::time_point;

auto constexpr now = &std::chrono::system_clock::now;

double elapsed(TimePoint tp);
double elapsed(TimePoint tp_begin, TimePoint tp_end);

struct AutoStopWatch
{
	explicit AutoStopWatch(const std::string& msg);
	~AutoStopWatch();
	double elapsed();

private:
	TimePoint start_;
	std::string msg_;
};

struct StopWatch
{
	explicit StopWatch(TimeDuration & td);
	~StopWatch();
	double elapsed();

private:
	TimePoint start_;
	TimeDuration& out_;
};

struct Timer
{
	explicit Timer(const TimeDuration& td);
	explicit Timer(int seconds);

	double remaining();
	bool alive();
private:
	TimePoint start_;
	TimeDuration wait_time_;
};

}
