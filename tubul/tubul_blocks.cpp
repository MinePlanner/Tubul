//
// Created by Carlos Acosta on 27-01-23.
//

#include <vector>
#include <unordered_map>
#include <variant>
#include <string>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <sstream>
#include "tubul_blocks.h"

#include <numeric>

#include "tubul_blocks.h"
#include "tubul_time.h"
#include "tubul_mem_utils.h"
#include "tubul_logger.h"
#include <format>


namespace TU
{
struct StringKey {
	std::variant<std::string, std::string_view> storage;

	StringKey(std::string s) : storage(std::move(s)) {}
	StringKey(std::string_view sv) : storage(sv) {}

	std::string_view view() const noexcept {
		return std::visit([](const auto& s) -> std::string_view { return s; }, storage);
	}
};

struct StringKeyHash {
	using is_transparent = void;
	size_t operator()(const StringKey& k) const noexcept {
		return std::hash<std::string_view>{}(k.view());
	}
	size_t operator()(std::string_view sv) const noexcept {
		return std::hash<std::string_view>{}(sv);
	}
	size_t operator()(const std::string& s) const noexcept {
		return std::hash<std::string_view>{}(s);
	}
};

struct StringKeyEqual {
	using is_transparent = void;
	bool operator()(const StringKey& a, const StringKey& b) const noexcept { return a.view() == b.view(); }
	bool operator()(std::string_view sv, const StringKey& k) const noexcept { return sv == k.view(); }
	bool operator()(const StringKey& k, std::string_view sv) const noexcept { return k.view() == sv; }
	bool operator()(const std::string& s, const StringKey& k) const noexcept { return s == k.view(); }
	bool operator()(const StringKey& k, const std::string& s) const noexcept { return k.view() == s; }
};

struct BlockDescription
{
	explicit BlockDescription(const std::string_view n) :
		name(n),
		allocAtStart(memLifetime()),
		start_time(now())
	{}

	std::string_view name;
	size_t    allocAtStart;
	TimePoint start_time;
};

std::vector<BlockDescription>& getBlockContainer()
{
	// Lambda trick to reserve size for a static vector.
	static std::vector<BlockDescription> block_container = []{
		// We don't think we are reaching more than 20 blocks deep
		// (remember block_container is a stack of active blocks)
		const size_t BLOCKS_DEPTH = 20;
		std::vector<BlockDescription> v;
		v.reserve(BLOCKS_DEPTH);
		return v;
	}();
	return block_container;
}

std::unordered_map<StringKey, BlockStats, StringKeyHash, StringKeyEqual>& getBlockStatsContainer()
{
	static std::unordered_map<StringKey, BlockStats, StringKeyHash, StringKeyEqual> stat_container;
	return stat_container;
}

void logBlockOnOpen(const BlockDescription& ) {
	logDevel(std::format("Starting {} |  mem rss/peak/alive: [{}/{}/{}]",
		getCurrentBlockLocation(), bytesToStr(memCurrentRSS()), bytesToStr(memPeakRSS()),
		bytesToStr(memAlive())));
}

void logBlockOnClose( const BlockDescription& b, TimeDuration block_duration, TimeDuration accum_duration) {
	//To store the amount of seconds as a double.
	auto allocations = memLifetime() - b.allocAtStart;
	logDevel(std::format("Closing {} |  rss/peak/alive/allocated: [{}/{}/{}/{}] e: {:g}s  accum:{:g}s",
		getCurrentBlockLocation(), bytesToStr(memCurrentRSS()), bytesToStr(memPeakRSS()),
		bytesToStr(memAlive()), bytesToStr(allocations), block_duration.count(), accum_duration.count()));
}

Block::Block(const std::string &name, LogType l):
	whenToLog_(l)
{
	auto& blocks = getBlockContainer();
	auto& blockStats = getBlockStatsContainer();
	auto it = blockStats.find(name);
	if(it == blockStats.end())
		it = blockStats.emplace(std::string{name}, BlockStats{}).first;
	index_ = blocks.size();
	blocks.emplace_back( it->first.view() );
	if ( l == LogType::ALL or l == LogType::ON_START)
		logBlockOnOpen(blocks.back());
}

Block::Block(std::string_view name, LogType l) :
	whenToLog_(l)
{
	auto& blocks = getBlockContainer();
	auto& blockStats = getBlockStatsContainer();
	auto it = blockStats.find(name);
	if (it == blockStats.end())
		it = blockStats.emplace(name, BlockStats{}).first;
	index_ = blocks.size();
	blocks.emplace_back( it->first.view() );
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
	auto& [ n, accum ] = getBlockStatsContainer().find(closingBlock.name)->second;
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
	constexpr size_t zero = 0UL;
	const size_t final_length = std::accumulate(blocks.begin(), blocks.end(), zero, sizeAcc);

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
	auto& [ n, accum ] = getBlockStatsContainer().find(reportingBlock.name)->second;

	auto allocations = memLifetime() - reportingBlock.allocAtStart;

	logReport(std::format("Reporting {} | rss/peak/alive/allocated: [{}/{}/{}/{}] e: {:g}s accum: {:g}s",
		getCurrentBlockLocation(), bytesToStr(memCurrentRSS()), bytesToStr(memPeakRSS()),
		bytesToStr(memAlive()), bytesToStr(allocations), block_duration.count(), accum.count()));

}
// generates table with a list of blocks, shows name and stats
std::string reportBlocks()
{
	auto &blocks = getBlockStatsContainer();

	size_t maxWidth = 0;
	for (const auto &[key, val] : blocks)
	{
		maxWidth = std::max(maxWidth, key.view().size());
	}

	std::ostringstream report;

	// header
	report << std::format("| {:<{}} | times created | accumulated time |\n", "name", maxWidth);

	for (const auto &[name, stats] : blocks)
	{
		auto n         = stats.count_;
		auto accum     = stats.t_;
		auto accumTime = std::to_string(accum.count());

		// I am assuming that there is not gonna be a gigantic number here
		report << std::format("| {:<{}} | {:<14}| {:<17}|\n", name.view(), maxWidth, n, accumTime);
	}
	return report.str();
}

// Returns the accumulated stats for a given block name (across all closed
// instances of the block name), or zeroed stats if the block was never recorded.
BlockStats getAccumulatedStats(const std::string& name)
{
	const auto &blocks = getBlockStatsContainer();
	auto it = blocks.find(name);
	if (it == blocks.end())
		return {};
	return it->second;
}


}
