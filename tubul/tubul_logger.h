//
// Created by Nicolas Loira on 2/3/23.
//

#pragma once

#include <cstdint>
#include <sstream>
#include "tubul_log_engine.h"
#include "tubul_exception.h"
#ifndef TUBUL_MACOS
#include <source_location>
#endif

namespace TU {



/** log* functions, allow to send a message to all loggers that
 * participate on the corresponding level
 * @param The message to be sent
 */
    void logError(std::string const &msg);
    void logWarning(std::string const &msg);
    void logReport(std::string const &msg);
    void logInfo(std::string const &msg);
    void logDevel(std::string const &msg);
    void logStat(std::string const &msg);
    void logDebug(std::string const &msg);

// ostream-like handler objects

    class LogStream {
    public:
        explicit LogStream(LogLevel level): level_(level) {}

        ~LogStream() {
                getLogEngineInstance().log(level_, parts_.str());
        };

        template<typename TypeToLog>
        LogStream& operator<<(TypeToLog&& msg ) {
            parts_ << msg;
           return *this;
        }

    private:
        std::stringstream parts_;

        LogLevel level_;
    };

    LogStream logError();
    LogStream logWarning();
    LogStream logReport();
    LogStream logInfo();
    LogStream logDevel();
    LogStream logStat();
    LogStream logDebug();

#ifdef TUBUL_MACOS
    [[nodiscard]] TU::Exception throwError(const std::string &msg, int line = __builtin_LINE(),
                                           const char *file = __builtin_FILE(),
                                           const char *function = __builtin_FUNCTION());
#else
    [[nodiscard]] TU::Exception throwError(const std::string &message,
												const std::source_location location =
													std::source_location::current());
#endif
}