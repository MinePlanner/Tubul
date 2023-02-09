//
// Created by Carlos Acosta on 30-01-23.
//

#pragma once
#include <string>
#include <vector>
#include <exception>

namespace TU
{

struct Exception : public std::runtime_error
{
	Exception(const std::string& msg);
	Exception& operator<<(const std::string& extra_info);
	std::string to_string() const;

private:
	std::vector<std::string> msgs_;
};


}