#pragma once

#include <string>
#include "tubul.h"

namespace TU
{

void initTubulApp(const std::string& name, const std::string& version);

const std::string& getAppName();
const std::string& getAppVer();

}
