//
// Created by Carlos Acosta on 27-01-23.
//

#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include "tubul_blocks.h"


namespace TU
{
struct BlockDescription
{
	using TimePoint = decltype(std::chrono::high_resolution_clock::now());
	BlockDescription(const std::string& n, TimePoint tp):
		name(n), start_time(tp)
	{}

	std::string name;
	TimePoint start_time;
};

std::vector<BlockDescription>& getBlockContainer()
{
	static std::vector<BlockDescription> block_container;
	return block_container;
}

ProcessBlock::ProcessBlock(const std::string &name)
{
	auto& blocks = getBlockContainer();
	index_ = blocks.size();
	blocks.emplace_back( name, std::chrono::high_resolution_clock::now() );
}

ProcessBlock::~ProcessBlock()
{
	auto& blocks = getBlockContainer();
	//If somehow the blocks are empty and we got here, that means somethign really
	//weird happened.
	assert(not blocks.empty());
	//We always just drop the last block in the stack given the way blocks
	//are supposed to be created.
	auto& closingBlock = blocks.back();
	//To store the amount of seconds as a double.
	using Duration = std::chrono::duration<double, std::ratio<1>>;
	Duration block_duration =  std::chrono::high_resolution_clock::now() - closingBlock.start_time ;
	//We should do something about the duration, like logging it for now just avoid
	//warnings for variables not used.
	(void) block_duration;
	blocks.pop_back();
	//Just to be safe, let's check the number of blocks is the.
	assert(index_ == blocks.size());
}

std::string getCurrentBlockLocation()
{
	//I'd really like to use join here...
	auto const& blocks = getBlockContainer();
	if (blocks.empty())
		return {};
	auto res = blocks.front().name;
	auto it = blocks.begin()+1;
	auto end = blocks.end();
	for (;it != end; ++it)
		res.append( std::string(" > ") + it->name);
	return res;
}


}