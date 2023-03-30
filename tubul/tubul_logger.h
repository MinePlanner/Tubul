//
// Created by Nicolas Loira on 2/3/23.
//

#pragma once

#include <cstdint>
#include "tubul_string.h"
#include "tubul_exception.h"
#ifndef TUBUL_MACOS
#include <source_location>
#endif

namespace TU {
    enum class LogLevel : uint8_t {
        ERROR,
        WARNING,
        REPORT,
        INFO,
        DEVEL,
        STATS,
        DEBUG
    };

    enum class LogOptions : uint8_t {
        NONE = 0,
        COLOR = 1,          // send commands for color output
        EXCLUSIVE = 2,      // only send to specified LogLevel
        NOTIMESTAMP = 4,    // don't prefix timestamp
        QUIET = 8           // don't show anything
    };

    inline LogOptions operator|(LogOptions a, LogOptions b) {
        return static_cast<LogOptions>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline uint8_t operator^(LogOptions a, LogOptions b) {
        return static_cast<uint8_t>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
    }

    inline uint8_t operator&(LogOptions a, LogOptions b) {
        return static_cast<uint8_t>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }


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

// ostream handler objects

    class LogStream {
    public:
        explicit LogStream(LogLevel level);

        ~LogStream();

        LogStream &operator<<(std::string const &msg);
        LogStream &operator<<(std::string_view const &msg);
        LogStream &operator<<(char const *msg);
        LogStream &operator<<(int const &msg);
        LogStream &operator<<(size_t const &msg);
        LogStream &operator<<(double const &msg);

    private:
        std::vector<std::string> parts_;
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