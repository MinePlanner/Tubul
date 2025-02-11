#pragma once

#include <string>

namespace TU
{

void initTubulApp(const std::string& name, const std::string& version);

const std::string& getAppName();
const std::string& getAppVer();

}
