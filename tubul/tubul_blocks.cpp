//
// Created by Carlos Acosta on 27-01-23.
//

#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include "tubul_blocks.h"

#include <tubul.h>

#include "tubul_time.h"
#include "tubul_mem_utils.h"
#include "tubul_logger.h"


namespace TU
{
struct BlockDescription
{
	explicit
	BlockDescription(const std::string& n):
		name(n),
		allocAtStart(memLifetime()),
		start_time(now())
	{}

	std::string name;
	size_t allocAtStart;
	TimePoint start_time;
};

std::vector<BlockDescription>& getBlockContainer()
{
	static std::vector<BlockDescription> block_container;
	return block_container;
}

void logBlockOnOpen(const BlockDescription& b) {
	logDevel() << "Starting " << getCurrentBlockLocation() << " | "
		<< " mem rss/peak/alive: [" << bytesToStr(memCurrentRSS()) << "/" << bytesToStr(memPeakRSS())
		<< "/" << bytesToStr(memAlive()) << "]";
}

void logBlockOnClose( const BlockDescription& b) {
	//To store the amount of seconds as a double.
	using Duration = std::chrono::duration<double, std::ratio<1>>;
	Duration block_duration =  now() - b.start_time ;
	auto allocations = memLifetime() - b.allocAtStart;
	logDevel() << "Closing " << getCurrentBlockLocation() << " | "
		<< " rss/peak/alive/allocated: [" << bytesToStr(memCurrentRSS()) << "/" << bytesToStr(memPeakRSS())
		<< "/" << bytesToStr(memAlive()) << "/" << bytesToStr(allocations)
		<<  "] e: " << block_duration.count() << "s";
}

Block::Block(const std::string &name):
	whenToLog_(LogType::ALL)
{
	auto& blocks = getBlockContainer();
	index_ = blocks.size();
	blocks.emplace_back( name );
	logBlockOnOpen(blocks.back());
}

Block::Block(const std::string &name, LogType l):
	whenToLog_(l)
{
	auto& blocks = getBlockContainer();
	index_ = blocks.size();
	blocks.emplace_back( name );
	if ( l == LogType::ALL or l == LogType::ON_START)
		logBlockOnOpen(blocks.back());
}

Block::~Block()
{
	auto& blocks = getBlockContainer();
	//If somehow the blocks are empty and we got here, that means somethign really
	//weird happened.
	assert(not blocks.empty());
	//We always just drop the last block in the stack given the way blocks
	//are supposed to be created.
	auto& closingBlock = blocks.back();
	if ( whenToLog_ == LogType::ALL or whenToLog_ == LogType::ON_END)
		logBlockOnClose( closingBlock );
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
