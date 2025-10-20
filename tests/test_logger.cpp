//
// Created by Nicolas Loira on 1/26/23.
//

#include "tubul.h"
#include <gtest/gtest.h>

TEST(TUBULLogger, testLogError)
{
	EXPECT_THROW(
		{
            TU::clearLoggerDefinitions();
            TU::addLoggerDefinition(
                    std::cout,
                    TU::LogLevel::INFO,
                    TU::LogOptions::QUIET);
			try
			{
				throw TU::throwError("test");
			}
			catch (const std::runtime_error &e)
			{
				EXPECT_EQ("Error: 'test' at function ", std::string(e.what()).substr(0, 26));
				throw;
			}
		},
		std::runtime_error);
}

TEST(TUBULLogger, testLogStreams)
{
    std::ostringstream oss;
    TU::clearLoggerDefinitions();
    TU::addLoggerDefinition(oss, TU::LogLevel::INFO, TU::LogOptions::NOTIMESTAMP);

    TU::logInfo() << "Hello Tubul";
    EXPECT_EQ(oss.str(), "Hello Tubul\n");

    TU::logWarning() << "Danger, Will Robinson!";
    EXPECT_EQ(oss.str(), "Hello Tubul\nDanger, Will Robinson!\n");

    TU::logError() << "Shutting Down Allegro.";
    EXPECT_EQ(oss.str(), "Hello Tubul\nDanger, Will Robinson!\nShutting Down Allegro.\n");
}

TEST(TUBULLogger, testLogWithTimestamp)
{
    std::ostringstream oss;
    TU::clearLoggerDefinitions();
    TU::addLoggerDefinition(oss, TU::LogLevel::INFO);

    TU::logInfo() << "Xello Tubul";
    std::string msg = oss.str().substr(22);
    EXPECT_EQ(msg , "Xello Tubul\n");
}

TEST(TUBULLogger, testLogExpclusive) {
    std::ostringstream infoexc;
    std::ostringstream info;

    TU::clearLoggerDefinitions();
    TU::addLoggerDefinition(infoexc, TU::LogLevel::INFO,
                           TU::LogOptions::EXCLUSIVE |
                           TU::LogOptions::NOTIMESTAMP);
    TU::addLoggerDefinition(info, TU::LogLevel::INFO, TU::LogOptions::NOTIMESTAMP);

    TU::logInfo() << "MessageInfo";
    TU::logWarning() << "MessageWarning";

    EXPECT_EQ(infoexc.str(), "MessageInfo\n");
    EXPECT_EQ(info.str(), "MessageInfo\nMessageWarning\n");
}


TEST(TUBULLogger, testLogDevel) {
    std::ostringstream ossReport;
    std::ostringstream ossDevel;

    TU::clearLoggerDefinitions();

    TU::addLoggerDefinition(ossReport, TU::LogLevel::REPORT, TU::LogOptions::NOTIMESTAMP);
    TU::addLoggerDefinition(ossDevel, TU::LogLevel::DEVEL, TU::LogOptions::NOTIMESTAMP);

    TU::logDevel() << "Message to Devel"; // should go to ossDevel only
    TU::logWarning() << "Danger, Will Robinson!"; // should go to both

    EXPECT_EQ(ossReport.str(), "Danger, Will Robinson!\n");
    EXPECT_EQ(ossDevel.str(), "Message to Devel\nDanger, Will Robinson!\n");
}
TEST(TUBULLogger, testLogCallback)
{
	std::stringstream ossReport;
    std::ostringstream ossDevel;
	auto logCallback = [&](TU::LogLevel logLevel, const std::string& msg)
	{
		if (logLevel == TU::LogLevel::REPORT)
			ossReport << msg;
		if (logLevel == TU::LogLevel::DEVEL)
			ossDevel << msg;
	};
	TU::clearLoggerDefinitions();
	TU::addLoggerDefinition(logCallback, TU::LogLevel::DEVEL);
	std::string msgReport = "Message to Report";
	std::string msgDevel = "Message to Devel";

	TU::logReport() << msgReport;
	TU::logDevel() << msgDevel;

	EXPECT_EQ(ossReport.str(), msgReport);
	EXPECT_EQ(ossDevel.str(), msgDevel);

}
