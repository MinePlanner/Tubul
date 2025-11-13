//
// Created by Nicolas Loira on 2/3/23.
//

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <functional>

#include "tubul_log_types.h"

namespace TU {

    int getVersion();

    /** Internal class that handles log-related functionality. It is not expected
     * the tubul users would deal directly with the LogEngine, and Tubul should expose the
     * functionality through other free helper functions to simplify usage.
     */
    class LogEngine {
    public:
    	//Wrapper to use callbacks provided from the outside.
    	using LogCallback = std::function<void(TU::LogLevel, const std::string& )>;

        LogEngine();

        ~LogEngine();

        //Functions to add a "log stream". We can add an user-defined stream-like object (via ostream reference)
        //or receive a string and create a new file that we will use as a log backend.
        void addLoggerDefinition(std::ostream &outLog, LogLevel level, LogOptions options = LogOptions::NONE);
        void addLoggerDefinition(const std::string &logFilename, LogLevel level, LogOptions options = LogOptions::NONE);
        void addLoggerDefinition(LogCallback callback, LogLevel level, LogOptions options = LogOptions::NONE);

        //Delete all existing logger streams
        void clearLoggerDefinitions();

        //Base functions to handle the logging requests. The safe version uses a mutex to prevent issues
        //while using the logger in threads.
        void log(LogLevel level, std::string const &text);

    private:
        //Simple structure to wrap an index from managed files vector
        struct ManagedFileIndex {
            size_t index_;
        };
        struct ManagedCallback {
            size_t index_;
        };

    	struct LogDispatchVisitor;

        //Loggers can point to a managed file or a user-provided stream.
        using LogStreamItem = std::variant<ManagedFileIndex, std::ostream*, ManagedCallback>;
        //The definition of a log is a LogStream (the variant we just defined) along with the level and options
        using LogDefinition = std::tuple< LogStreamItem, TU::LogLevel, TU::LogOptions>;

        //Open a file and stores the created stream. Returns an index (pretty much like a C fd) to be used later.
        size_t openFile(std::string const &fileName);
        //Given a LogStreamItem, returns the associated std::ostream&
        std::ostream& getLogStream(const LogStreamItem& item) ;

        std::vector<std::ofstream> managedFiles_;
    	std::vector<LogCallback> managedCallbacks_;
        std::vector<LogDefinition> loggers_;

        bool loggerDefined_;
    };

    /** Function to get the global LogEngine in tubul. Most functions should access logging
     * features using this getter to ensure they are using the one and only engine that is
     * configured with the user-facing functions.
     */
    LogEngine &getLogEngineInstance();
} // namespace TU