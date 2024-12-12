//
// Created by Nicolas Loira on 2/3/23.
//

#include "tubul_log_engine.h"
#include <mutex>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace TU {

    int getVersion(){ return 0; }

    LogEngine::LogEngine() :
            loggerDefined_(true) {

        // by default, create a logger to stdout with level logInfo
        // override after first addLogDefinition!
        addLoggerDefinition(std::cout, LogLevel::INFO, LogOptions::NOTIMESTAMP);
        loggerDefined_ = false;

    }

    LogEngine::~LogEngine() {
        // close managed files
        while (not managedFiles_.empty())
            managedFiles_.pop_back();
    }

    size_t LogEngine::openFile(std::string const &fileName) {
        size_t ret = managedFiles_.size();
        managedFiles_.emplace_back(fileName);
        return ret;
    }

   std::ostream& LogEngine::getLogStream(const LogStreamItem& item) {
        if ( std::holds_alternative<ManagedFileIndex>(item)) {
            auto &fileIdx = std::get<ManagedFileIndex>(item);
            return managedFiles_[fileIdx.index_];
        }
        if ( std::holds_alternative<std::ostream*>(item)) {
            auto ptr = std::get<std::ostream*>(item);
            return *ptr;
        }
        throw std::runtime_error("Unknown logger stream type");
    }

    void LogEngine::addLoggerDefinition(std::ostream &outLog, LogLevel level, LogOptions options) {
        // first logger definition overrides default behavior
        if (not loggerDefined_) {
            loggers_.clear();
            loggerDefined_ = true;
        }
        LogStreamItem item = std::addressof(outLog);

        loggers_.emplace_back(item, level, options);
    }

    void LogEngine::addLoggerDefinition(const std::string &outLogFilename, LogLevel level, LogOptions options) {
        // first logger definition overrides default behavior
        if (not loggerDefined_) {
            loggers_.clear();
            loggerDefined_ = true;
        }
        ManagedFileIndex item = { openFile(outLogFilename) };
        LogStreamItem logItem = item;

        loggers_.emplace_back(logItem, level, options);
    }

    void LogEngine::clearLoggerDefinitions(){
        loggers_.clear();
        loggerDefined_ = true;
    }

    void LogEngine::log(LogLevel level, std::string const &text) {

#ifdef NDEBUG
        if (level == LogLevel::DEBUG)
            return;
#endif

        for (auto &[logWrapper, loggerLevel, options] : loggers_) {
            std::ostream &logStream = getLogStream(logWrapper);

            if (level > loggerLevel)
                continue;

            if (options & LogOptions::QUIET)
                continue;

            if ((options & LogOptions::EXCLUSIVE) and (level != loggerLevel))
                continue;

            if (not (options & LogOptions::NOTIMESTAMP))
                streamTimestamp(logStream);

            logStream << text;
            if (text.empty() or (text.back() != '\n'))
                logStream << std::endl;
            else
                logStream << std::flush;
        }
    }

    void LogEngine::safelog(LogLevel level, std::string const &text) {
        static std::mutex iolock;
        std::scoped_lock<std::mutex> l(iolock);
        log(level, text);
    }

    void LogEngine::streamTimestamp(std::ostream &logStream) {
        auto now = std::chrono::system_clock::now();
        time_t tnow = std::chrono::system_clock::to_time_t(now);
        tm buf;
#ifdef TUBUL_WINDOWS
        //Windows bad people changed the api!!
        localtime_s(&buf, &tnow);
        const auto* utc = &buf;
#else
        tm *utc = localtime_r(&tnow, &buf);
#endif

        logStream << std::setfill('0');
        logStream << std::setw(4) << utc->tm_year + 1900; // Year
        logStream << '-';
        logStream << std::setw(2) << utc->tm_mon + 1; // Month
        logStream << '-';
        logStream << std::setw(2) << utc->tm_mday; // Day
        logStream << ' ';
        logStream << std::setw(2) << utc->tm_hour << ':'; // Hours
        logStream << std::setw(2) << utc->tm_min << ':';  // Minutes
        logStream << std::setw(2) << utc->tm_sec;         // Seconds
        logStream << " - ";
    }

    LogEngine &getLogEngineInstance() {
        static LogEngine engine;
        return engine;
    }
} // namespace TU
