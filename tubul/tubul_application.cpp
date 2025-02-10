#include "tubul_application.h"
#include "string"

namespace TU
{

struct TubulApplication
{
	TubulApplication() = default;

	std::string _program_name;
	std::string _program_version;
};


TubulApplication& getApp()
{
	static TubulApplication app;
	return app;
}
void initTubulApp(const std::string& name, const std::string& version){
	TubulApplication& app = getApp();
	app._program_name = name;
	app._program_version = version;
}

const std::string& getAppName()
{
	TubulApplication& app = getApp();
	return app._program_name;
}
const std::string& getAppVer()
{
	TubulApplication& app = getApp();
	return app._program_version;
}

}

