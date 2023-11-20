//
// Created by Nicolas Loira on 5/22/21.
//

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include "neoparams_types.h"
#include "INIReader.h"


namespace NeoParams{


class NeoParamsData {

public:
	explicit NeoParamsData(std::istream& input);
    explicit NeoParamsData(const std::string& input);

	// set a value for a param key
	template<typename T>
	void set(const std::string &key, const T value)
	{
		std::string lowerkey = tolower(key);
		m_paramQueues[lowerkey].back() = ParamValue(value);
	}

	// set from a string, using type defined on paramdef
	void setFromString(std::string const &key, std::string const &value);

	// get param value
	template<typename T>
	T get(const std::string &anykey)
	{
		std::string key = tolower(anykey);

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
		std::string key = tolower(anykey);
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
	std::unordered_map<std::string, NeoParamsType> m_paramType;

	// Primary set of queues
	std::unordered_map<std::string, std::vector<ParamValue>> m_paramQueues;

};


void init(std::istream& input);
void init(const std::string& input);

NeoParamsData &getInstance();

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
