#pragma once

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
	NONE        = 0,
	COLOR       = 1, // send commands for color output
	EXCLUSIVE   = 2, // only send to specified LogLevel
	NOTIMESTAMP = 4, // don't prefix timestamp
	QUIET       = 8  // don't show anything
};

inline LogOptions operator|(LogOptions a, LogOptions b)
{
	using underType = std::underlying_type_t<LogOptions>;
	return static_cast<LogOptions>(static_cast<underType>(a) | static_cast<underType>(b));
}

inline auto operator^(LogOptions a, LogOptions b)
{
	using underType = std::underlying_type_t<LogOptions>;
	return static_cast<underType>(static_cast<underType>(a) ^ static_cast<underType>(b));
}

inline auto operator&(LogOptions a, LogOptions b)
{
	using underType = std::underlying_type_t<LogOptions>;
	return static_cast<underType>(static_cast<underType>(a) & static_cast<underType>(b));
}
} // namespace TU
