//
// Created by Nicolas Loira on 1/11/23.
//

#include "tubul.h"
#include "tubul_engine.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

#ifndef TUBUL_MACOS
#include <source_location>
#endif

namespace TU
{

#ifdef TUBUL_MACOS
[[nodiscard]] std::runtime_error throwError(const std::string &msg, int line, const char *file, const char *function)
{
	std::string errormsg =
		std::string("Error: '") + msg + "' at function " + function + " (" + file + ":" + std::to_string(line) +
		")";
	// this should go to logger:
	// std::cout << errormsg << std::endl;
	return std::runtime_error(errormsg);
}
#else
[[nodiscard]] std::runtime_error throwError(const std::string         &message,
										  const std::source_location location)
{
	std::string errormsg = std::string("Error: '") + message + "' at function " + location.function_name() + " (" + location.file_name() + ":" + std::to_string(location.line()) + ")";
	return std::runtime_error(errormsg);
}
#endif

void addLoggerDefinition(std::string const &logfile, TU::LogLevel level, TU::LogOptions options)
{
	std::ostream &outStream = Tubul::getInstance().openFile(logfile);
	addLoggerDefinition(outStream, level, options);
}

void addLoggerDefinition(std::ostream &out, TU::LogLevel level, TU::LogOptions options)
{
	Tubul::getInstance().addLoggerDefinition(out, level, options);
}

void logInfo(std::string const &msg)
{
	Tubul::getInstance().log(LogLevel::INFO, msg);
}

void logReport(std::string const &msg)
{
	Tubul::getInstance().log(LogLevel::REPORT, msg);
}

void logWarning(std::string const &msg)
{
	Tubul::getInstance().log(LogLevel::WARNING, msg);
}


} // namespace TU
