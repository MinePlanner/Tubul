//
// Created by Nicolas Loira on 5/22/21.
//

#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <variant>
#include "tubul_string.h"
#include "INIReader.h"
#include "json.hpp"


namespace TU::Parameters{

//Entry points to the params functionality from the outside.
//We either initialize the engine from a stream(likely a file)
//or directly from a string.
void init(std::istream& input);
void init(const std::string& input);

enum ParamType {
	INT,
	FLOAT,
	STRING,
	BOOL,
	LIST_INT,
	LIST_FLOAT
};

// possible values to store on neoparams engine
using ParamValue = std::variant<
	int,
	double,
	bool,
	std::string,
	std::vector<int>,
	std::vector<double>
	>;

class ParamsData {

public:
	explicit ParamsData(std::istream& input);
    explicit ParamsData(const std::string& input);

	// set a value for a param key
	template<typename T>
	void set(const std::string &key, const T value)
	{
		std::string lowerkey = TU::tolower(key);
		m_paramQueues[lowerkey].back() = ParamValue(value);
	}

	// set from a string, using type defined on paramdef
	void setFromString(std::string const &key, std::string const &value);

	// get param value
	template<typename T>
	T get(const std::string &anykey)
	{
		std::string key = TU::tolower(anykey);

		if (not m_paramQueues.count(key))
			throw std::runtime_error("Trying to get a nonexisting parameter " + anykey);

		ParamValue &pv = m_paramQueues[key].back();
		if (std::holds_alternative<T>(pv))
			return std::get<T>(pv);

		throw std::runtime_error("Trying to obtain value of " + anykey + " as incorrect type");
	}

	// push new param value (will be used for future get() calls)
	template<typename T>
	void push(const std::string &anykey, const T &value)
	{
		std::string key = TU::tolower(anykey);
		m_paramQueues[key].push_back(ParamValue(value));
	}

	// remove the last pushed value for param key
	void pop(const std::string &key);

	// show current/queues
	std::string printCurrentValues();
    std::string printAllValues();


private:

    void startup();


	// paramdef from json
	nlohmann::json m_paramDef;

	// type for each param, from paramdef
	std::unordered_map<std::string, ParamType> m_paramType;

	// Primary set of queues
	std::unordered_map<std::string, std::vector<ParamValue>> m_paramQueues;

};



ParamsData &getInstance();

void loadFromFile(const std::string& filename);
void loadFromString(const std::string& contents);

template<typename T>
T get(const std::string &key)
{
    return getInstance().get<T>(key);
}

template<typename T>
void set(const std::string &key, T value)
{
    getInstance().set<T>(key, value);
}

template<typename T>
void push(const std::string &key, T value)
{
    return getInstance().push<T>(key, value);
}

inline
void pop(const std::string &key)
{
    return getInstance().pop(key);
}
inline
std::string dumpParams(){
    return getInstance().printCurrentValues();
}
inline
std::string dumpAllParams(){
    return getInstance().printAllValues();
}


}//end namespace neoparams
