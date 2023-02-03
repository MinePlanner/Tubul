//
// Created by Nicolas Loira on 2/3/23.
//

#pragma once

#include "tubul_logger.h"
#include <string>
#include <vector>
namespace TU
{

class TubulEngine
{
public:
	TubulEngine();
	~TubulEngine();

	std::ostream &openFile(std::string const &fileName);
	void          addLoggerDefinition(std::ostream &outLog, LogLevel level, LogOptions options);
	void          log(LogLevel level, std::string const &text);

private:
	std::vector<std::ofstream> managedFiles_;
	std::vector<
		std::tuple<
			std::reference_wrapper<std::ostream>,
			TU::LogLevel,
			TU::LogOptions>>
		loggers_;
};

} // namespace TU