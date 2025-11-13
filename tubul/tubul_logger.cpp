//
// Created by Nicolas Loira on 1/11/23.
//

#include "tubul_log_engine.h"
#include "tubul_logger.h"
#include "tubul_exception.h"
#include "tubul_string.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

#ifndef TUBUL_MACOS
#include <source_location>
#endif

namespace TU {


    void addLoggerDefinition(std::ostream &out, TU::LogLevel level, TU::LogOptions options) {
        getLogEngineInstance().addLoggerDefinition(out, level, options);
    }

    void addLoggerDefinition(std::string const &logfile, TU::LogLevel level, TU::LogOptions options) {
        getLogEngineInstance().addLoggerDefinition(logfile, level, options);
    }

    void addLoggerDefinition(std::function<void(TU::LogLevel, const std::string&)> callback, TU::LogLevel level, TU::LogOptions options) {
        getLogEngineInstance().addLoggerDefinition(std::move(callback), level, options);
    }

    void clearLoggerDefinitions(){
        getLogEngineInstance().clearLoggerDefinitions();
    }

    void logInfo(std::string const &msg) {
        getLogEngineInstance().log(LogLevel::INFO, msg);
    }

    void logReport(std::string const &msg) {
        getLogEngineInstance().log(LogLevel::REPORT, msg);
    }

    void logWarning(std::string const &msg) {
        getLogEngineInstance().log(LogLevel::WARNING, "WARNING: " + msg);
    }

    void logError(std::string const &msg) {
        getLogEngineInstance().log(LogLevel::ERROR, "ERROR: " + msg);
    }

    void logDebug(std::string const &msg) {
        getLogEngineInstance().log(LogLevel::DEBUG, "DEBUG: " + msg);
    }

    void logDevel(std::string const &msg) {
        getLogEngineInstance().log(LogLevel::DEVEL, "DEVEL: " + msg);
    }

    void logStat(std::string const &msg) {
        getLogEngineInstance().log(LogLevel::STATS, "STATS: " + msg);
    }

#ifdef TUBUL_MACOS

    [[nodiscard]] TU::Exception throwError(const std::string &msg, int line, const char *file, const char *function) {
        std::string errormsg =
                std::string("Error: '") + msg + "' at function " + function + " (" + file + ":" + std::to_string(line) +
                ")";
        logError(errormsg);
        return {errormsg};
    }

#else
    [[nodiscard]] TU::Exception throwError(const std::string         &message,
                                          const std::source_location location)
{
    std::string errormsg = std::string("Error: '") + message + "' at function " + location.function_name() + " (" + location.file_name() + ":" + std::to_string(location.line()) + ")";
    logError(errormsg);
    return TU::Exception(errormsg);
}
#endif



    LogStream logInfo() {
        return LogStream(LogLevel::INFO);
    }

    LogStream logReport() {
        return LogStream(LogLevel::REPORT);
    }

    LogStream logWarning() {
        return LogStream(LogLevel::WARNING);
    }

    LogStream logError() {
        return LogStream(LogLevel::ERROR);
    }

    LogStream logDevel() {
        return LogStream(LogLevel::DEVEL);
    }

    LogStream logStat() {
        return LogStream(LogLevel::STATS);
    }

    LogStream logDebug() {
        return LogStream(LogLevel::DEBUG);
    }

}; // namespace TU
