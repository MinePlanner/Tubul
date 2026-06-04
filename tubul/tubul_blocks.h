//
// Created by Carlos Acosta on 27-01-23.
//

#pragma once
#include <string>
#include "tubul_time.h"
#define TUBUL_BLOCK TU::Block ___aux_t_block(__FUNCTION__)

namespace TU
{

struct BlockStats
{
	BlockStats():
		count_(0),
		t_(TimeDuration::zero())
	{}

	size_t count_;
	TimeDuration t_;
};

struct Block
{
	enum class LogType
	{
		ON_START,
		ON_END,
		ALL,
		NONE
	};

	template<size_t N>
	explicit Block(const char (&name)[N]) : Block(std::string_view{name, N-1}, LogType::ALL) {}
	template<size_t N>
	Block(const char (&name)[N], LogType l) : Block(std::string_view{name, N-1}, l) {}
	explicit Block(const std::string& name) : Block(name, LogType::ALL) {}
	Block(const std::string& name, LogType l);
	Block(std::string_view name, LogType l);
	~Block();
	void report();

private:
	size_t index_;
	LogType whenToLog_;
};

std::string getCurrentBlockLocation();

std::string reportBlocks();

BlockStats getAccumulatedStats(const std::string& name);

}
