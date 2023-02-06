//
// Created by Nicolas Loira on 2/3/23.
//

#pragma once

#include "tubul.h"
#include <cstdint>

namespace TU
{
enum class LogLevel : uint8_t
{
	ERROR,
	WARNING,
	REPORT,
	INFO,
	DEVEL,
	STATS,
	DEBUG
};

enum class LogOptions : uint8_t
{
	NONE		= 0,
	COLOR       = 1,
	EXCLUSIVE   = 2,
	NOTIMESTAMP = 4
};

inline LogOptions operator|(LogOptions a, LogOptions b)
{
	return static_cast<LogOptions>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline uint8_t operator^(LogOptions a, LogOptions b)
{
	return static_cast<uint8_t>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
}

// ostream handler objects

class LogWarning{
public:
	LogWarning() = default;;
	~LogWarning() {
		logWarning(join(parts_, ""));
	};
	LogWarning *operator<<(std::string const &msg)
	{
		parts_.push_back(msg);
		return this;
	}

private:
	std::vector<std::string> parts_;
};

}