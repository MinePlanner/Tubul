//
// Created by Nicolas Loira on 2/3/23.
//

#pragma once

#include <string>
#include <vector>

#include "tubul_logger.h"

namespace TU {

    int getVersion();

    class TubulEngine {
    public:
        TubulEngine();

        ~TubulEngine();

        std::ostream &openFile(std::string const &fileName);

        void addLoggerDefinition(std::ostream &outLog, LogLevel level, LogOptions options = LogOptions::NONE);

        void clearLoggerDefinitions();

        void log(LogLevel level, std::string const &text);

    private:
        static void streamTimestamp(std::ostream &logStream);

        std::vector<std::ofstream> managedFiles_;
        std::vector<
                std::tuple<
                        std::reference_wrapper<std::ostream>,
                        TU::LogLevel,
                        TU::LogOptions>>
                loggers_;

        bool loggerDefined_;
    };

} // namespace TU