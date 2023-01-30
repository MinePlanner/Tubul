//
// Created by Carlos Acosta on 27-01-23.
//

#include <vector>
#include <string>
#include <iostream>
#include "tubul_blocks.h"


namespace TU
{
std::vector<std::string>& getBlockContainer()
{
	static std::vector<std::string> block_container;
	return block_container;
}

ProcessBlock::ProcessBlock(const std::string &name)
{
	auto& blocks = getBlockContainer();
	index_ = blocks.size();
	blocks.push_back(name);
	std::cout << "Adding block " << name << std::endl;
}

ProcessBlock::~ProcessBlock()
{
	auto& blocks = getBlockContainer();
	std::cout << "deleting block '" << blocks.at(index_) << "'" << std::endl;
	blocks.pop_back();
}

std::string getCurrentBlockLocation()
{
	//I'd really like to use join here...
	auto const& blocks = getBlockContainer();
	if (blocks.empty())
		return std::string();
	std::string res = blocks.front();
	auto it = blocks.begin()+1;
	auto end = blocks.end();
	for (;it != end; ++it)
		res.append( std::string(" > ") + *it);
	return res;


}


}