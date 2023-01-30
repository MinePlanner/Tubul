//
// Created by Carlos Acosta on 27-01-23.
//

#pragma once
#include <string>

namespace TU
{

struct ProcessBlock
{
	ProcessBlock(const std::string& name);
	~ProcessBlock();

private:
	size_t index_;
};


}