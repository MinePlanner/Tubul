//
// Created by Carlos Acosta on 27-01-23.
//

#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <cassert>
#include "tubul_blocks.h"

#include <numeric>
#include <tubul.h>

#include "tubul_time.h"
#include "tubul_mem_utils.h"
#include "tubul_logger.h"
#include <iomanip>


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

struct BlockStats {
	BlockStats():
		count_(0),
		t_(TimeDuration::zero())
	{}

	size_t count_;
	TimeDuration t_;
};

std::vector<BlockDescription>& getBlockContainer()
{
	static std::vector<BlockDescription> block_container;
	return block_container;
}

std::unordered_map<std::string, BlockStats>& getBlockStatsContainer()
{
	static std::unordered_map<std::string, BlockStats> stat_container;
	return stat_container;
}

void logBlockOnOpen(const BlockDescription& b) {
	logDevel() << "Starting " << getCurrentBlockLocation() << " | "
		<< " mem rss/peak/alive: [" << bytesToStr(memCurrentRSS()) << "/" << bytesToStr(memPeakRSS())
		<< "/" << bytesToStr(memAlive()) << "]";
}

void logBlockOnClose( const BlockDescription& b, TimeDuration block_duration, TimeDuration accum_duration) {
	//To store the amount of seconds as a double.
	auto allocations = memLifetime() - b.allocAtStart;
	logDevel() << "Closing " << getCurrentBlockLocation() << " | "
		<< " rss/peak/alive/allocated: [" << bytesToStr(memCurrentRSS()) << "/" << bytesToStr(memPeakRSS())
		<< "/" << bytesToStr(memAlive()) << "/" << bytesToStr(allocations)
		<<  "] e: " << block_duration.count() << "s  accum:" << accum_duration.count() << "s";
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
	//We calculate how much time has passed since the creation of this block.
	TimeDuration block_duration =  now() - closingBlock.start_time ;
	//Add info to the mapped data of this particular block
	auto& [ n, accum ] = getBlockStatsContainer()[closingBlock.name];
	++n;
	accum += block_duration;

	if ( whenToLog_ == LogType::ALL or whenToLog_ == LogType::ON_END)
		logBlockOnClose( closingBlock, block_duration, accum);

	blocks.pop_back();
	//Just to be safe, let's check the number of blocks is the.
	assert(index_ == blocks.size());
}

std::string getCurrentBlockLocation()
{
	//I'd really like to use join here...
	//The string i will use as a joiner can be defined just once.
	static const std::string joiner(".");

	//Get the current blocks, but if we don't have anything, we can't do
	//anything else.
	auto const& blocks = getBlockContainer();
	if (blocks.empty())
		return {};

	//Simple lambda to accumulate length of a bunch of strings.
	auto sizeAcc = [](size_t current, const BlockDescription& b) {
		return current + b.name.size();
	};
	//Get the length of all names
	const size_t final_length = std::accumulate(blocks.begin(), blocks.end(),0 , sizeAcc);

	//We will start with an empty string that will hold the final concatenated location.
	//This strings needs a total space of the sum of all block name's length plus the number
	//of join charaters required. After getting the space, we store the first name. After
	//getting the space for the final string, we store the first name.
	std::string res;
	res.reserve(final_length + ( joiner.size() * (blocks.size() -1) ) );
	res.append(blocks.front().name);
	//Then we add all the rest appending the joiner and the name
	auto it = blocks.begin()+1;
	auto end = blocks.end();
	for (;it != end; ++it) {
		res.append( joiner );
		res.append( it->name );
	}
	return res;
}
void Block::report(){
	auto& blocks = getBlockContainer();
	auto& reportingBlock = blocks[index_];
	//We calculate how much time has passed since the creation of this block.
	TimeDuration block_duration =  now() - reportingBlock.start_time ;
	//Current info
	auto& [ n, accum ] = getBlockStatsContainer()[reportingBlock.name];

	auto allocations = memLifetime() - reportingBlock.allocAtStart;

	logReport() << "Reporting " << getCurrentBlockLocation() << " |" 
		<< " rss/peak/alive/allocated: [" << bytesToStr(memCurrentRSS()) << "/"
		<< bytesToStr(memPeakRSS()) << "/" << bytesToStr(memAlive()) << "/" << bytesToStr(allocations)
		<< "] e: " << block_duration.count() << "s accum: " << accum.count() << "s";

}
//generates table with a list of blocks, shows name and stats
std::string reportBlocks(){
	//map of stats
	auto& blocks = getBlockStatsContainer();

	auto begin = blocks.begin(), end = blocks.end();
	//for decent table
	size_t width = 0;
	for(auto it = begin; it != end; ++it){
		width = std::max(it->first.size(), width);
	}
	
	std::ostringstream report;

	//header
	report << std::left << "| " << std::setw(width) << "name" << " |" << " times created " << "|" << " accumulated time " << "|\n";
	
	for(auto it = begin;it != end; ++it){
		std::ostringstream buf;

		std::string name = it->first;

		auto n = it->second.count_;
		auto accum = it->second.t_;
		
		std::string times = std::to_string(n), accumTime = std::to_string(accum.count());

		//I am assuming that there is not gonna be a gigantic number here
		report << std::left << "| " << std::setw(width) << name << " | " << std::setw(14) << n <<"| " << std::setw(17) << accumTime << "|\n";
	}	
	return report.str();
}


}
