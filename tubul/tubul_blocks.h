//
// Created by Carlos Acosta on 27-01-23.
//

#pragma once
#include <string>

namespace TU
{

struct Block
{
	explicit Block(const std::string& name);
	~Block();

private:
	size_t index_;
};


}