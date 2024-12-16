//
// Created by Nicolas Loira on 5/22/21.
//

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <regex>
#include <exception>

#include "tubul_params_impl.h"

namespace TU::Parameters{


namespace {
    std::unique_ptr<ParamsData> global_instance = nullptr;
}



// utils
ParamValue getParamDefDefault(nlohmann::json &paramdef, ParamType paramsType)
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

ParamType getParamDefType(nlohmann::json &paramdef)
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

    if (stringToType.count(dataType) == 0)
        throw std::runtime_error("Unrecognized type on paramdef: " + dataType);

    return stringToType.at(dataType);
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
    std::string vectorToString(const std::vector<T>& pv) const {
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
    std::string operator()(const bool pv) const { return simpleToString(pv);}
    std::string operator()(const std::string& pv) const { return pv;}
    std::string operator()(const std::vector<int>& pv) const { return vectorToString(pv); }
    std::string operator()(const std::vector<double>& pv) const { return vectorToString(pv); }

};

std::string toString(ParamValue const &pv)
{
    return std::visit(ParamValuePrinter(), pv);
}


void init(std::istream& input)
{
    global_instance = std::make_unique<ParamsData>(input);
}

void init(const std::string& input)
{
    global_instance = std::make_unique<ParamsData>(input);
}

ParamsData& getInstance()
{
    if (global_instance != nullptr)
        return *global_instance;

    throw;
}

ParamsData::ParamsData(std::istream& input):
        m_paramDef(nlohmann::json::parse(input)){
    startup();
}

ParamsData::ParamsData(const std::string& input):
        m_paramDef(nlohmann::json::parse(input)){
    startup();
}

void ParamsData::startup()
{
    if (m_paramDef.empty())
        throw;

    for (auto &[section, paramdefs] : m_paramDef.items()) {
        for (auto &paramdef : paramdefs) {
            std::string full_name = tolower(section) + "." + tolower(paramdef["name"].get<std::string>());

            ParamType paramType = getParamDefType(paramdef);
            m_paramType[full_name] = paramType;
            m_paramQueues[full_name].push_back(getParamDefDefault(paramdef, paramType));
        }
    }
}


void loadFromFile(const std::string& paramFile)
{
	ParamsData &params = Parameters::getInstance();

	auto reader = INIReader::fromFile(paramFile);

	for(auto &value: reader.Values())
	{
		std::string keyName(value.first);

		// replace INIReader keys (section=key) with local format (section.key)
		std::size_t posEqual = keyName.find_first_of('=');
		if (posEqual==std::string::npos)
			throw std::runtime_error("param oddly defined: " + keyName + "\n");
		keyName[posEqual] = '.';

		params.setFromString(keyName, value.second);
	}
}

void loadFromString(const std::string& paramString)
{
    ParamsData &params = Parameters::getInstance();

    auto reader = INIReader::fromString(paramString);

    for(auto &value: reader.Values())
    {
        std::string keyName(value.first);

        // replace INIReader keys (section=key) with local format (section.key)
        std::size_t posEqual = keyName.find_first_of('=');
        if (posEqual==std::string::npos)
            throw std::runtime_error("param oddly defined: " + keyName + "\n");
        keyName[posEqual] = '.';

        params.setFromString(keyName, value.second);
    }
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

	if (m_paramType.count(key) == 0)
		throw std::runtime_error("Trying to set an unknown key: "+key);

	ParamType paramType = m_paramType[key];

	switch(paramType)
	{
		case INT: set<int>(key, std::stoi(value)); break;
		case FLOAT: set<double>(key, std::stod(value)); break;
		case BOOL: set<bool>(key, validTrue.find(value) != validTrue.end()); break;
		case STRING: set<std::string>(key, value); break;
		case LIST_INT: set<std::vector<int>>(key, parseIntList(value)); break;
		case LIST_FLOAT: set<std::vector<double>>(key, parseFloatList(value)); break;
	}
}




std::string ParamsData::printCurrentValues() {

    std::stringstream out;
	out << "---------------------\n";
	for (const auto &[key, valueList] : m_paramQueues)
		out << key << ": " << toString(valueList.back()) << "\n";

    return out.str();
}

std::string ParamsData::printAllValues() {

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

} //end namespace neoparams
