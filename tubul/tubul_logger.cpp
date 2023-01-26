//
// Created by Nicolas Loira on 1/11/23.
//

#include "tubul.h"
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

} // namespace TU
