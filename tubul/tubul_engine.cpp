//
// Created by Nicolas Loira on 2/3/23.
//

#include "tubul_engine.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace TU {

    int getVersion(){ return 0; }

    TubulEngine::TubulEngine() :
            loggerDefined_(true) {
        // this is the init() of Tubul

        // by default, create a logger to stdout with level logInfo
        // override after first addLogDefinition!
        addLoggerDefinition(std::cout, LogLevel::INFO, LogOptions::NOTIMESTAMP);
        loggerDefined_ = false;

    }

    TubulEngine::~TubulEngine() {
        // close managed files
        while (not managedFiles_.empty())
            managedFiles_.pop_back();
    }

    std::ostream &TubulEngine::openFile(std::string const &fileName) {
        std::ofstream outStream(fileName);
        managedFiles_.push_back(std::move(outStream));
        return managedFiles_.back();
    }

    void TubulEngine::addLoggerDefinition(std::ostream &outLog, LogLevel level, LogOptions options) {
        // first logger definition overrides default behavior
        if (not loggerDefined_) {
            loggers_.clear();
            loggerDefined_ = true;
        }

        loggers_.emplace_back(outLog, level, options);
    }

    void TubulEngine::clearLoggerDefinitions(){
        loggers_.clear();
        loggerDefined_ = true;
    }

    void TubulEngine::log(LogLevel level, std::string const &text) {

#ifdef NDEBUG
        if (level == LogLevel::DEBUG)
            return;
#endif

        for (auto &logDefinition: loggers_) {
            std::ostream &logStream = std::get<0>(logDefinition);
            LogLevel loggerLevel = std::get<1>(logDefinition);
            LogOptions options = std::get<2>(logDefinition);

            if (level > loggerLevel)
                continue;

            if (options & LogOptions::QUIET) {
                continue;
            }

            if (options ^ LogOptions::NOTIMESTAMP)
                streamTimestamp(logStream);

            logStream << text;
            if (text.empty() or (text.back() != '\n'))
                logStream << std::endl;
            else
                logStream << std::flush;
        }
    }

    void TubulEngine::streamTimestamp(std::ostream &logStream) {
        auto now = std::chrono::system_clock::now();
        time_t tnow = std::chrono::system_clock::to_time_t(now);
        tm *utc = localtime(&tnow);

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

} // namespace TU