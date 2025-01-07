//
// Created by Carlos Acosta on 27-01-23.
//

#pragma once
#include <string>

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


}