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

    void safelogError(std::string const &msg);
    void safelogWarning(std::string const &msg);
    void safelogReport(std::string const &msg);
    void safelogInfo(std::string const &msg);
    void safelogDevel(std::string const &msg);
    void safelogStat(std::string const &msg);
    void safelogDebug(std::string const &msg);

// ostream-like handler objects

    struct LogStreamNormalTag;
    struct LogStreamThreadSafeTag;

    template <typename Tag>
    concept LogStreamTag =
        std::is_same_v<Tag, LogStreamNormalTag> or
        std::is_same_v<Tag, LogStreamThreadSafeTag>;

    template <LogStreamTag TagType>
    class LogStream {
    public:
        explicit LogStream(LogLevel level): level_(level) {}

        ~LogStream() {
            if constexpr (std::is_same_v<TagType, LogStreamNormalTag> )
                getLogEngineInstance().log(level_, parts_.str());
            else if constexpr ( std::is_same_v<TagType, LogStreamThreadSafeTag> )
                getLogEngineInstance().safelog(level_, parts_.str());

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
    using LogStreamU = LogStream<LogStreamNormalTag>;
    using LogStreamTS = LogStream<LogStreamThreadSafeTag>;

    LogStreamU logError();
    LogStreamU logWarning();
    LogStreamU logReport();
    LogStreamU logInfo();
    LogStreamU logDevel();
    LogStreamU logStat();
    LogStreamU logDebug();
    LogStreamTS safelogError();
    LogStreamTS safelogWarning();
    LogStreamTS safelogReport();
    LogStreamTS safelogInfo();
    LogStreamTS safelogDevel();
    LogStreamTS safelogStat();
    LogStreamTS safelogDebug();

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