//
// Created by Carlos Acosta on 30-06-23.
//

#pragma once

#include "tubul_string.h"
#include "tubul_types.h"
#include "json_fwd.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

class INIReader;

namespace TU
{

enum ParamType
{
	INT,
	FLOAT,
	STRING,
	BOOL,
	LIST_INT,
	LIST_FLOAT
};

struct ParamStorage
{
	// possible values to store on neoparams engine
	using ParamValue = std::variant<
		int,
		double,
		bool,
		std::string,
		std::vector<int>,
		std::vector<double>>;

	ParamStorage() = default;

	// Reset any parameter configuration
	void clear();

	// Passes the configuration of parameters to parse.
	void defineParams(std::string_view paramsConfig);
	void defineParams(std::istream &paramsConfig);

	// load a params file (as many times as you want)
	void loadParamsFile(const std::string &paramsFile);

	// load a params file (as many times as you want)
	void loadParamsString(const std::string &content);

	// set a value for a param key
	template <typename T>
	void set(std::string_view key, const T value);

	// set a string param from a string_view (std::string's constructor from
	// string_view is explicit, so the variant can't do this conversion on its own)
	void set(std::string_view key, std::string_view value);

	// get param value
	template <typename T>
	T get(std::string_view anykey);

	// set from a string, using type defined on paramdef
	void setFromString(std::string_view key, std::string_view value);

	// push new param value (will be used for future get() calls)
	template <typename T>
	void push(std::string_view anykey, const T &value);

	// remove the last pushed value for param key
	void pop(const std::string &key);

	// return several params to their previous values
	void pop(const std::vector<std::string> &listOfParams);

	// show current/queues
	std::string printCurrentValues() const;
	std::string printAllValues() const;

	// return the default-defined value for the given parameter
	template <typename T>
	T getDefault(std::string_view param);

	// return if the given parameter is currently using its default-defined value
	bool usingDefault(std::string_view param);

	// return the help texts ("description" on paramdef) for all parameters
	const StringMap<std::string> &getHelpTexts() const;

	// return a copy of a section, where keys are at the root level
	// (should be recovered with "key", not with "section.key")
	//    Parameters getSection(std::string_view& section);

	// get a copy All parameters
	//    Parameters &getAllParameters();

	// get parameters that differs from default
	//    Parameters &getNonDefaultParameters();

private:
	// shared implementation for the defineParams overloads, works over the parsed json
	void defineParamsImpl(const nlohmann::json &paramDef);

	// type for each param, from paramdef
	StringMap<ParamType> m_paramType;

	// Primary set of queues
	StringMap<std::vector<ParamValue>> m_paramQueues;

	// to store default-defined values
	StringMap<ParamValue> m_defaultValues;

	// to store a quick "help text" for each param
	StringMap<std::string> m_helpText;
};

template <typename T>
void ParamStorage::set(std::string_view key, const T value)
{
	m_paramQueues[TU::tolower(key)].back() = ParamValue(value);
}

inline void ParamStorage::set(std::string_view key, std::string_view value)
{
	m_paramQueues[TU::tolower(key)].back() = ParamValue(std::string(value));
}

template <typename T>
void ParamStorage::push(std::string_view anykey, const T &value)
{
	m_paramQueues[TU::tolower(anykey)].push_back(ParamValue(value));
}

template <typename T>
T ParamStorage::get(std::string_view anykey)
{
	std::string key = TU::tolower(anykey);
	auto found = m_paramQueues.find(key);
	if (found == m_paramQueues.end())
		throw std::runtime_error("Trying to get a nonexisting parameter " + std::string(anykey));
	ParamValue &pv = found->second.back();
	if (std::holds_alternative<T>(pv))
		return std::get<T>(pv);
	throw std::runtime_error("Trying to obtain value of " + std::string(anykey) + " as incorrect type");
}

template <typename T>
T ParamStorage::getDefault(std::string_view anykey)
{
	std::string key = TU::tolower(anykey);
	auto found = m_defaultValues.find(key);
	if (found == m_defaultValues.end())
		throw std::runtime_error("Trying to get the default value of a nondefined parameter " + std::string(anykey));
	ParamValue &pv = found->second;
	if (std::holds_alternative<T>(pv))
		return std::get<T>(pv);
	throw std::runtime_error("Trying to obtain default value of " + std::string(anykey) + " as incorrect type");
}

} // end namespace TU
