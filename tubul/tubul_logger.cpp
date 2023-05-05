//
// Created by Nicolas Loira on 1/11/23.
//

#include "tubul_engine_factory.h"
#include "tubul_logger.h"
#include "tubul_exception.h"
#include "tubul_string.h"

#include <fstream>
#include <iostream>
#include <utility>

#ifndef TUBUL_MACOS
#include <source_location>
#endif

namespace TU {


    void addLoggerDefinition(std::ostream &out, TU::LogLevel level, TU::LogOptions options) {
        getInstance().addLoggerDefinition(out, level, options);
    }

    void addLoggerDefinition(std::string const &logfile, TU::LogLevel level, TU::LogOptions options) {
        std::ostream &outStream = getInstance().openFile(logfile);
        addLoggerDefinition(outStream, level, options);
    }

    void clearLoggerDefinitions(){
        getInstance().clearLoggerDefinitions();
    }

    void logInfo(std::string const &msg) {
        getInstance().log(LogLevel::INFO, msg);
    }

    void logReport(std::string const &msg) {
        getInstance().log(LogLevel::REPORT, msg);
    }

    void logWarning(std::string const &msg) {
        getInstance().log(LogLevel::WARNING, "WARNING: " + msg);
    }

    void logError(std::string const &msg) {
        getInstance().log(LogLevel::ERROR, "ERROR: " + msg);
    }

    void logDebug(std::string const &msg) {
        getInstance().log(LogLevel::DEBUG, "DEBUG: " + msg);
    }

    void logDevel(std::string const &msg) {
        getInstance().log(LogLevel::DEVEL, "DEVEL: " + msg);
    }

    void logStat(std::string const &msg) {
        getInstance().log(LogLevel::STATS, "STATS: " + msg);
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

    LogStream::LogStream(LogLevel level) : level_(level) {};

    LogStream::~LogStream() {
        getInstance().log(level_, join(std::as_const(parts_), ""));
    };

    LogStream &LogStream::operator<<(std::string const &msg) {
        parts_.push_back(msg);
        return *this;
    }

    LogStream &LogStream::operator<<(std::string_view const &msg) {
        parts_.emplace_back(msg);
        return *this;
    }

    LogStream &LogStream::operator<<(char const *msg){
        parts_.emplace_back(msg);
        return *this;
    }

    LogStream &LogStream::operator<<(int const &msg) {
        parts_.push_back(std::to_string(msg));
        return *this;
    }

    LogStream &LogStream::operator<<(size_t const &msg) {
        parts_.push_back(std::to_string(msg));
        return *this;
    }

    LogStream &LogStream::operator<<(double const &msg) {
        parts_.push_back(std::to_string(msg));
        return *this;
    }

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

    LogStream logStats() {
        return LogStream(LogLevel::STATS);
    }

}; // namespace TU
