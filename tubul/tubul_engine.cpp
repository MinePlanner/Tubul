//
// Created by Nicolas Loira on 2/3/23.
//

#include "tubul_engine.h"
#include <algorithm>
#include <fstream>

namespace TU
{

TubulEngine::TubulEngine()
{
	// this is the init() of Tubul
}

TubulEngine::~TubulEngine()
{
	// close managed files
	while (not managedFiles_.empty())
		managedFiles_.pop_back();
}

std::ostream &TubulEngine::openFile(std::string const &fileName)
{
	std::ofstream outStream(fileName);
	managedFiles_.push_back(std::move(outStream));
	return managedFiles_.back();
}

void TubulEngine::addLoggerDefinition(std::ostream &outLog, LogLevel level, LogOptions options)
{
	loggers_.emplace_back(outLog, level, options);
}

void TubulEngine::log(LogLevel level, std::string const &text)
{
	for (auto &logDefinition : loggers_)
	{
		if (level > std::get<1>(logDefinition))
			continue;

		std::ostream &logStream = std::get<0>(logDefinition);
		LogOptions    options   = std::get<2>(logDefinition);

		if (options ^ LogOptions::NOTIMESTAMP)
		{
			logStream << "YYYY-MM-DD HH:MM:SS - ";
		}

		logStream << text;
		if (text.empty() or (text.back() != '\n'))
			logStream << std::endl;
		else
			logStream << std::flush;
	}
}

} // namespace TU