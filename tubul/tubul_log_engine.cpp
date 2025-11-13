//
// Created by Nicolas Loira on 2/3/23.
//

#include "tubul_log_engine.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace TU {

static
    auto getFormattedTimestamp() {

		using namespace std::chrono;
        auto nowTime = system_clock::now();
        auto tnow = system_clock::to_time_t(nowTime);
		std::tm buf;
#ifdef TUBUL_WINDOWS
        //Windows bad people changed the api!!
        localtime_s(&buf, &tnow);
#else
		localtime_r(&tnow, &buf);
#endif

		std::ostringstream oss;
		oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S - ") ;
		return oss.str();
	}

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
        LogStreamItem item(std::in_place_type<std::ostream*>, std::addressof(outLog));

        loggers_.emplace_back(item, level, options);
    }

    void LogEngine::addLoggerDefinition(const std::string &outLogFilename, LogLevel level, LogOptions options) {
        // first logger definition overrides default behavior
        if (not loggerDefined_) {
            loggers_.clear();
            loggerDefined_ = true;
        }
        ManagedFileIndex item = { openFile(outLogFilename) };
        LogStreamItem logItem(std::in_place_type<ManagedFileIndex>, item);

        loggers_.emplace_back(logItem, level, options);
    }

    void LogEngine::addLoggerDefinition(LogCallback callback, LogLevel level, LogOptions options) {
        // first logger definition overrides default behavior
        if (not loggerDefined_) {
            loggers_.clear();
            loggerDefined_ = true;
        }
        ManagedCallback item = { managedCallbacks_.size()};
        managedCallbacks_.emplace_back(std::move(callback));
        LogStreamItem logItem(std::in_place_type<ManagedCallback>, item);

        loggers_.emplace_back(logItem, level, options);
    }

    void LogEngine::clearLoggerDefinitions(){
        loggers_.clear();
        loggerDefined_ = true;
    }

	struct LogEngine::LogDispatchVisitor
	{
		void handleStream(std::ostream *sPtr) const
		{
			static std::mutex iolock;
			std::scoped_lock<std::mutex> l(iolock);

			std::ostream &out = *sPtr;
			if (useTimestampFlag)
				out << timestamp;
			out << text;
			if (text.empty() or (text.back() != '\n'))
				out << std::endl;
			else
				out << std::flush;
		}
		void operator()(ManagedFileIndex &idx) const
		{
			auto aux = std::addressof(engine.managedFiles_[idx.index_]);
			handleStream(aux);
		}

		void operator()(std::ostream *sPtr) const
		{
			handleStream(sPtr);
		}

		void operator()(ManagedCallback& idx)
		{
			auto& callback = engine.managedCallbacks_[idx.index_];

			static std::mutex cblock;
			std::scoped_lock<std::mutex> l(cblock);
			callback(level, text);
		}

		LogEngine         &engine;
		LogLevel           level;
		const std::string &text;
		const std::string &timestamp;
		bool               useTimestampFlag;
	};

	void LogEngine::log(LogLevel level, std::string const &text)
	{
#ifdef NDEBUG
		if (level == LogLevel::DEBUG)
			return;
#endif

		std::string timestamp;
		auto        useTimestamp = [](LogOptions options)
		{ return (not(options & LogOptions::NOTIMESTAMP)); };

		for (auto &[logWrapper, loggerLevel, options] : loggers_)
		{
			if (level > loggerLevel)
				continue;

			if (options & LogOptions::QUIET)
				continue;

			if ((options & LogOptions::EXCLUSIVE) and (level != loggerLevel))
				continue;

			if (useTimestamp(options))
			{
				timestamp = getFormattedTimestamp();
			}
			// lambda that helps us dispatch to the actual backends of each logItem
			LogDispatchVisitor dispatch{*this, level, text, timestamp, useTimestamp(options)};
			std::visit(dispatch, logWrapper);
		}
	}

    LogEngine &getLogEngineInstance() {
        static LogEngine engine;
        return engine;
    }
} // namespace TU
