//
// Created by Carlos Acosta on 27-01-23.
//

#pragma once
#include <string>
#define TUBUL_BLOCK TU::Block ___aux_t_block(__FUNCTION__)

namespace TU
{

struct Block
{
	enum class LogType
	{
		ON_START,
		ON_END,
		ALL,
		NONE
	};

	explicit Block(const std::string& name);
	Block(const std::string& name, LogType l);
	~Block();
	void report();

private:
	size_t index_;
	LogType whenToLog_;
};

std::string getCurrentBlockLocation();

std::string reportBlocks();

}
