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

    void safelogInfo(std::string const &msg) {
        getLogEngineInstance().safelog(LogLevel::INFO, msg);
    }

    void safelogReport(std::string const &msg) {
        getLogEngineInstance().safelog(LogLevel::REPORT, msg);
    }

    void safelogWarning(std::string const &msg) {
        getLogEngineInstance().safelog(LogLevel::WARNING, "WARNING: " + msg);
    }

    void safelogError(std::string const &msg) {
        getLogEngineInstance().safelog(LogLevel::ERROR, "ERROR: " + msg);
    }

    void safelogDebug(std::string const &msg) {
        getLogEngineInstance().safelog(LogLevel::DEBUG, "DEBUG: " + msg);
    }

    void safelogDevel(std::string const &msg) {
        getLogEngineInstance().safelog(LogLevel::DEVEL, "DEVEL: " + msg);
    }

    void safelogStat(std::string const &msg) {
        getLogEngineInstance().safelog(LogLevel::STATS, "STATS: " + msg);
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



    LogStreamU logInfo() {
        return LogStreamU(LogLevel::INFO);
    }

    LogStreamU logReport() {
        return LogStreamU(LogLevel::REPORT);
    }

    LogStreamU logWarning() {
        return LogStreamU(LogLevel::WARNING);
    }

    LogStreamU logError() {
        return LogStreamU(LogLevel::ERROR);
    }

    LogStreamU logDevel() {
        return LogStreamU(LogLevel::DEVEL);
    }

    LogStreamU logStat() {
        return LogStreamU(LogLevel::STATS);
    }

    LogStreamU logDebug() {
        return LogStreamU(LogLevel::DEBUG);
    }

    LogStreamTS safelogInfo() {
        return LogStreamTS(LogLevel::INFO);
    }

    LogStreamTS safelogReport() {
        return LogStreamTS(LogLevel::REPORT);
    }

    LogStreamTS safelogWarning() {
        return LogStreamTS(LogLevel::WARNING);
    }

    LogStreamTS safelogError() {
        return LogStreamTS(LogLevel::ERROR);
    }

    LogStreamTS safelogDevel() {
        return LogStreamTS(LogLevel::DEVEL);
    }

    LogStreamTS safelogStat() {
        return LogStreamTS(LogLevel::STATS);
    }

    LogStreamTS safelogDebug() {
        return LogStreamTS(LogLevel::DEBUG);
    }

}; // namespace TU
