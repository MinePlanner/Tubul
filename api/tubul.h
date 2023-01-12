//
// Created by Nicolas Loira on 11-01-23.
//

#pragma once
#include <vector>
#include <string>
#include <string_view>

namespace TU{
    void init();
    int getVersion();


	std::vector< std::string_view > split(std::string const& input, std::string const& delims);
}