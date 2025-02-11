// Created by Carlos Acosta on 30-06-23.
//
//


#include <unordered_map>
#include <string>
#include <variant>
#include <iostream>
#include <sstream>
#include <regex>
#include <cstdio>
#include "tubul_params.h"
#include "tubul_string.h"
#include "INIReader.h"
#include "json.hpp"

namespace TU
{

namespace Parameters
{

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


// utils
ParamValue getParamDefDefault(const nlohmann::json &paramdef, ParamType paramsType)
{
    ParamValue data;

    switch (paramsType) {
        case ParamType::INT: data = paramdef.contains("default") ? paramdef["default"].get<int>() : 0; break;
        case ParamType::FLOAT: data = paramdef.contains("default") ? paramdef["default"].get<double>() : 0.0; break;
        case ParamType::BOOL: data = paramdef.contains("default") && paramdef["default"].get<bool>(); break;
        case ParamType::STRING: data = paramdef.contains("default") ? paramdef["default"].get<std::string>() : std::string(""); break;
        case ParamType::LIST_INT: data = paramdef.contains("default") ? paramdef["default"].get<std::vector<int>>() : std::vector<int>(); break;
        case ParamType::LIST_FLOAT: data = paramdef.contains("default") ? paramdef["default"].get<std::vector<double>>() : std::vector<double>(); break;
    }

    return data;
}

ParamType getParamDefType(const nlohmann::json &paramdef)
{
    static const std::unordered_map<std::string, ParamType> stringToType = {
            {"int", INT},
            {"float", FLOAT},
            {"bool", BOOL},
            {"str", STRING},
            {"list_int", LIST_INT},
            {"list_float", LIST_FLOAT}
    };

    std::string dataType;
    if (not paramdef.count("type")) {
        dataType = "int";
    } else {
        dataType = paramdef["type"];
    }

    if (not stringToType.contains(dataType))
        throw std::runtime_error("Unrecognized type on paramdef: " + dataType);

    return stringToType.at(dataType);
}


struct ParamsData {

	ParamsData() = default;

	static ParamsData& getInstance()
	{
		static ParamsData params;
		return params;
	}

	// reset stores params and values
	void clear()
	{
		m_paramType.clear();
		m_paramQueues.clear();
	}

	// set a value for a param key
	template<typename T>
	void set(const std::string &key, const T value)
	{
		std::string lowerkey = TU::tolower(key);
		m_paramQueues[lowerkey].back() = ParamValue(value);
	}

	// set from a string, using type defined on paramdef
	void setFromString(std::string const &key, std::string const &value);


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
	std::string printCurrentValues() const;
    std::string printAllValues() const;

	// get param value
	template<typename T>
	T get(const std::string &anykey)
	{
		std::string key = TU::tolower(anykey);

		if (not m_paramQueues.contains(key))
			throw std::runtime_error("Trying to get a nonexisting parameter " + anykey);

		ParamValue &pv = m_paramQueues[key].back();
		if (std::holds_alternative<T>(pv))
			return std::get<T>(pv);

		throw std::runtime_error("Trying to obtain value of " + anykey + " as incorrect type");
	}

	template<typename T>
	T getDefault(const std::string& anykey)
	{
		std::string key = TU::tolower(anykey);

		if (not m_paramQueues.contains(key))
			throw std::runtime_error("Trying to get the default value of a nondefined parameter " + anykey);

		ParamValue _default = m_defaultValues[key];
		if (std::holds_alternative<T>(_default))
			return std::get<T>(_default);

		throw std::runtime_error("Trying to obtain default value of " + anykey + " as incorrect type");
	}

	bool usingDefault(const std::string &anykey)
	{
		std::string key = TU::tolower(anykey);

		if (not m_paramQueues.contains(key))
			throw std::runtime_error("Nondefined parameter " + anykey);

		ParamValue _default = m_defaultValues[key], _current = m_paramQueues[key].back();

		return _default == _current;

	}

    template <typename T>
    void addParamsConfig(T &&input)
    {
      nlohmann::json paramDef(nlohmann::json::parse(std::forward<T>(input)));
      if (paramDef.empty())
        throw std::runtime_error("No parameters were specified in configParams");

      for (const auto &[section, paramdefs] : paramDef.items()) {
        for (const auto &paramdef : paramdefs) {

          nlohmann::json theName = paramdef["name"];
          std::string jsonValue = theName.get<std::string>();
          auto lowerJson = tolower(jsonValue);

          std::string full_name = tolower(section) + "." + lowerJson;

          ParamType paramType = getParamDefType(paramdef);
          ParamValue paramValue = getParamDefDefault(paramdef, paramType);

          if (m_paramQueues.contains(full_name))
          {
            if (m_paramType[full_name] == paramType and m_paramQueues[full_name].back() == paramValue)
              continue;

            throw std::runtime_error("Trying to re-define already defined parameter " + full_name);
          }

          m_paramType[full_name] = paramType;

          m_paramQueues[full_name].push_back(paramValue);
          m_defaultValues[full_name] = paramValue;
        }
      }
    }

	void loadFromINIReader(INIReader &reader)
	{
		for (auto &value : reader.Values())
		{
			std::string keyName(value.first);

			//replace INIReader keys (section=key) with local format (section.key)
			std::size_t posEqual = keyName.find_first_of('=');
			if (posEqual == std::string::npos)
				throw std::runtime_error("param oddly defined: " + keyName + "\n");
			keyName[posEqual] = '.';

			setFromString(keyName, value.second);
		}
	}

private:

	// type for each param, from paramdef
	std::unordered_map<std::string, ParamType> m_paramType;

	// Primary set of queues
	std::unordered_map<std::string, std::vector<ParamValue>> m_paramQueues;

	// to store default-defined values
	std::unordered_map<std::string, ParamValue> m_defaultValues;

};

inline
std::string dumpParams(){
    return ParamsData::getInstance().printCurrentValues();
}
inline
std::string dumpAllParams(){
    return ParamsData::getInstance().printAllValues();
}

/** Helper visitor to convert the ParamValue into a string by handling the
 * real type of the param (but from the outside is "just another param").
 */
struct ParamValuePrinter{
    //Params are either single values or lists, so we can route all types
    //to either "simpleToString" or "vectorToString".
    template<typename T>
    std::string simpleToString(T pv) const { return std::to_string(pv);}

    template<typename T>
    std::string vectorToString(const std::vector<T>& pv) const
    {
        if (pv.empty())
            return "[ ]";
        std::stringstream ss;
        ss << "[";
        copy(pv.begin(), pv.end() - 1, std::ostream_iterator<T>(ss, ", "));
        ss << pv.back() << "]";
        return ss.str();
    }

    std::string operator()(const int pv) const{ return simpleToString(pv);}
    std::string operator()(const double pv) const { return simpleToString(pv);}
    std::string operator()(const bool pv) const { return (pv)?"true":"false";}
    std::string operator()(const std::string& pv) const { return pv;}
    std::string operator()(const std::vector<int>& pv) const { return vectorToString(pv); }
    std::string operator()(const std::vector<double>& pv) const { return vectorToString(pv); }

};

std::string toString(ParamValue const &pv)
{
    return std::visit(ParamValuePrinter(), pv);
}

//////////////////////////////////////////////////////////////////////
// NeoParamsData
//////////////////////////////////////////////////////////////////////
std::vector<std::string> getListTokens(std::string const& list)
{
  std::vector<std::string> res;

  //Checking that it actually looks like a list and get "contents"
  std::regex contents(R"(\[(.*)\])");
  std::smatch content_match;
  if (not std::regex_search(list, content_match, contents ))
  {
    //It didn't look like a list enclosed by brackets!
    return res;
  }

  //if we matched, the match #1 should have the csv items.
  std::string list_items = content_match[1];

  //now we use the trick to "walk over the delimiters" by
  //using a regex that has spaces or comma
  std::regex delim(R"([\s,]+)");
  std::smatch tokens;
  std::sregex_token_iterator it(list_items.begin(), list_items.end(), delim, -1);
  std::sregex_token_iterator end;
  for (; it != end; ++it)
  {
    res.emplace_back(it->str());
  }

  //And return the result
  return res;
}

std::vector<int> parseIntList(std::string const& val)
{
  //Get the tokenized line and convert to number.
  std::vector<int> res;
  auto tokens = getListTokens(val);
  for (const auto& it: tokens)
  {
    res.emplace_back(std::stoi(it));
  }

  return res;
}

std::vector<double> parseFloatList(std::string const& val)
{
  //Get the tokenized line and convert to number.
  std::vector<double> res;
  auto tokens = getListTokens(val);
  for (const auto& it: tokens)
  {
    res.emplace_back(std::stod(it));
  }
  return res;
}

void ParamsData::setFromString(std::string const &key, std::string const &value)
{
	// try to convert value to the type defined on paramdef
	std::set<std::string> validTrue = {"yes", "true", "True", "1"};

	auto paramType_it = m_paramType.find(key);

	if (paramType_it == m_paramType.end())
		throw std::runtime_error("Trying to set an unknown key: " + key);

	switch(paramType_it->second)
	{
		case INT: set<int>(key, std::stoi(value)); break;
		case FLOAT: set<double>(key, std::stod(value)); break;
		case BOOL: set<bool>(key, validTrue.find(value) != validTrue.end()); break;
		case STRING: set<std::string>(key, value); break;
		case LIST_INT: set<std::vector<int>>(key, parseIntList(value)); break;
		case LIST_FLOAT: set<std::vector<double>>(key, parseFloatList(value)); break;
	}
}

std::string ParamsData::printCurrentValues() const
{

    std::stringstream out;
	out << "---------------------\n";
	for (const auto &[key, valueList] : m_paramQueues)
		out << key << ": " << toString(valueList.back()) << "\n";

    return out.str();
}

std::string ParamsData::printAllValues() const
{
    std::stringstream out;
    out << "---------------------\n";
    for (const auto &[key, valueList] : m_paramQueues) {
        out << key << ": [";
        auto it = valueList.begin();
        out << toString(*it);
        for (++it; it != valueList.end(); ++it) {
            out << ", " << toString(*it) ;
        }
        out << "]\n";
    }
    return out.str();
}

void ParamsData::pop(const std::string &anykey)
{
	std::string key = tolower(anykey);
	m_paramQueues[key].pop_back();
}

}//end namespace parameters


void clearParams()
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	params.clear();
}

void addParamsConfig(std::istream& paramsConfig)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.addParamsConfig(paramsConfig);
}

void addParamsConfig(const std::string& paramsConfig)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.addParamsConfig(paramsConfig);
}

// load a params file (as many times as you want)
void loadParams(const std::string& paramsFile)
{
	INIReader reader = INIReader::fromFile(paramsFile);

	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.loadFromINIReader(reader);
}

// load a params string (as many times as you want)
void loadParamsString(const std::string& content)
{
	INIReader reader = INIReader::fromString(content);

	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	params.loadFromINIReader(reader);
}

// return to the previous value of a param
void popParam(const std::string& param)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.pop(param);
}

// return several params to their previous values
void popParams(const std::vector<std::string>& listOfParams)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    for (const auto& p: listOfParams){
        params.pop(p);
    }
}

// get the current value of a param
template <> int getParam(const std::string& param)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    return params.get<int>(param);
}

template <> double getParam(const std::string& param)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    return params.get<double>(param);
}

template <> bool getParam(const std::string& param)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    return params.get<bool>(param);
}

template <> std::string getParam(const std::string& param)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    return params.get<std::string>(param);
}

template <> std::vector<int> getParam(const std::string& param)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    return params.get<std::vector<int>>(param);
}

template <> std::vector<double> getParam(const std::string& param)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    return params.get<std::vector<double>>(param);
}


//Set the param to a new value
void setParam(const std::string& param, int value)
{ 
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.set<int>( param, value);
}

void setParam(const std::string& param, double value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.set<double>( param, value);
}

void setParam(const std::string& param, bool value) 
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.set<bool>( param, value);
}

void setParam(const std::string& param, const std::string& value) 
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.set<std::string>( param, value);
}

void setParam(const std::string& param, const char* value) 
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.set<std::string>( param, std::string(value) );
}

void setParam(const std::string& param, const std::vector<int>& value) 
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.set<std::vector<int>>( param, value);
}

void setParam(const std::string& param, const std::vector<double>& value) 
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.set<std::vector<double>>( param, value);
}

//Set the param to a temporary new value
void pushParam(const std::string& param, int value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.push(param,value);
}

void pushParam(const std::string& param, double value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.push(param,value);
}

void pushParam(const std::string& param, bool value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.push(param,value);
}

void pushParam(const std::string& param, const char* value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.push(param,std::string(value));
}

void pushParam(const std::string& param, const std::string& value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.push(param,value);
}

void pushParam(const std::string& param, const std::vector<int>& value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.push(param,value);
}

void pushParam(const std::string& param, const std::vector<double>& value)
{
    Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
    params.push(param,value);
}

template <> int getDefault(const std::string& param)
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	return params.getDefault<int>(param);
}

template <> double getDefault(const std::string& param)
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	return params.getDefault<double>(param);
}

template <> bool getDefault(const std::string& param)
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	return params.getDefault<bool>(param);
}

template <> std::string getDefault(const std::string& param)
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	return params.getDefault<std::string>(param);
}

template <> std::vector<int> getDefault(const std::string& param)
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	return params.getDefault<std::vector<int>>(param);
}

template <> std::vector<double> getDefault(const std::string& param)
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	return params.getDefault<std::vector<double>>(param);
}

bool usingDefault(const std::string &param)
{
	Parameters::ParamsData &params = Parameters::ParamsData::getInstance();
	return params.usingDefault(param);
}

void dumpParams(std::ostream& out){
    out << Parameters::dumpParams();
}
void dumpAllParams(std::ostream& out){
    out << Parameters::dumpAllParams();
}

}//end namespace TU
