//
// Created by Nicolas Loira on 1/11/23.
//

#include "tubul.h"
#include <stdexcept>

#ifndef TUBUL_MACOS
#include <source_location>
#endif

namespace TU {

#ifdef TUBUL_MACOS

    [[nodiscard]] std::runtime_error logError(const std::string &msg, int line, const char *file, const char *function) {
        std::string errormsg =
                std::string("Error: '") + msg + "' at function " + function + " (" + file + ":" + std::to_string(line) +
                ")";
        // logger.logReport(errormsg);
        return std::runtime_error(errormsg);
    }

#else

    std::runtime_error logError(const std::string_view message,
              const std::source_location location =
              std::source_location::current())
     {
         std::string errormsg = std::string("Error: '") + msg + "' at function " + function + " (" + file + ":" + std::to_string(line) + ")";
         // logger.logReport(errormsg);
         return std::runtime_error(errormsg);
     }
#endif


} // namespace TU
