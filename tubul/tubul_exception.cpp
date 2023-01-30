//
// Created by Carlos Acosta on 30-01-23.
//
#include "tubul_exception.h"
#include <vector>

namespace TU {

Exception::Exception(const std::string &msg):
	std::runtime_error(msg)
{
}


Exception& Exception::operator<<(const std::string &extra_info)
{
	msgs_.push_back(extra_info);
	return *this;
}

std::string Exception::to_string() const
{
	std::string res = what();
	res += "\n";
	for (const auto& msg: msgs_)
	{
		res.append(msg);
		res.append("\n");
	}
	return res;
}


}