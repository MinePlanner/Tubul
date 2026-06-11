// Created by Carlos Acosta on 30-06-23.
//
//


#include <unordered_map>
#include <string>
#include <variant>
#include <sstream>
#include <regex>
#include <cstdio>
#include "tubul_params.h"
#include "tubul_string.h"
#include "tubul_file_utils.h"
#include "INIReader.h"
#include "json.hpp"

namespace TU
{

using ParamValue = ParamStorage::ParamValue;

//
// Utils
//
namespace{
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

std::vector<std::string> getListTokens(std::string_view list)
{
  std::vector<std::string> res;

  //Checking that it actually looks like a list and get "contents"
  std::regex contents(R"(\[(.*)\])");
  std::match_results<std::string_view::const_iterator> content_match;
  if (not std::regex_search(list.begin(), list.end(), content_match, contents ))
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

std::vector<int> parseIntList(std::string_view val)
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

std::vector<double> parseFloatList(std::string_view val)
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

void loadFromINIReader(ParamStorage &storage, INIReader &reader)
{
	for (auto &value : reader.Values())
	{
		std::string keyName(value.first);

		//replace INIReader keys (section=key) with local format (section.key)
		std::size_t posEqual = keyName.find_first_of('=');
		if (posEqual == std::string::npos)
			throw std::runtime_error("param oddly defined: " + keyName + "\n");
		keyName[posEqual] = '.';

		storage.setFromString(keyName, value.second);
	}
}

} // end utils (anonymous namespace)

void ParamStorage::defineParamsImpl(const nlohmann::json &paramDef)
{
    if (paramDef.empty())
        throw std::runtime_error("No parameters were specified in configParams");

    for (const auto &[section, paramdefs] : paramDef.items()) {
        for (const auto &paramdef : paramdefs) {
            std::string full_name = tolower(section) + "." + tolower(paramdef["name"].get<std::string>());
            ParamType paramType = getParamDefType(paramdef);
            ParamValue paramValue = getParamDefDefault(paramdef, paramType);

            if (m_paramQueues.contains(full_name)) {
                if (m_paramType[full_name] == paramType && m_paramQueues[full_name].back() == paramValue)
                    continue;
                throw std::runtime_error("Trying to re-define already defined parameter " + full_name);
            }

            m_paramType[full_name] = paramType;
            m_paramQueues[full_name].push_back(paramValue);
            m_defaultValues[full_name] = paramValue;
            m_helpText[full_name] = paramdef.contains("description") ? paramdef["description"].get<std::string>() : std::string();
        }
    }
}

void ParamStorage::defineParams(std::string_view input)
{
    defineParamsImpl(nlohmann::json::parse(input));
}

void ParamStorage::defineParams(std::istream &input)
{
    defineParamsImpl(nlohmann::json::parse(input));
}

void ParamStorage::clear()
{
	m_paramType.clear();
	m_paramQueues.clear();
	m_helpText.clear();
}

const StringMap<std::string> &ParamStorage::getHelpTexts() const
{
	return m_helpText;
}


bool ParamStorage::usingDefault(std::string_view anykey)
{
	std::string key = TU::tolower(anykey);

	if (not m_paramQueues.contains(key))
		throw std::runtime_error("Nondefined parameter " + std::string(anykey));

	ParamValue _default = m_defaultValues[key], _current = m_paramQueues[key].back();

	return _default == _current;

}

// load a params file (as many times as you want)
void ParamStorage::loadParamsFile(const std::string& paramsFile)
{
	INIReader reader = INIReader::fromFile(paramsFile);
	loadFromINIReader(*this, reader);
}

// load a params string (as many times as you want)
void ParamStorage::loadParamsString(const std::string& content)
{
	INIReader reader = INIReader::fromString(content);
	loadFromINIReader(*this, reader);
}


void ParamStorage::setFromString(std::string_view inKey, std::string_view value)
{
	// try to convert value to the type defined on paramdef
	static const std::set<std::string, std::less<>> validTrue = {"yes", "true", "True", "1"};

	// keys are stored lowercased (see defineParamsImpl), so lookups must match
	std::string key = tolower(inKey);
	auto paramType_it = m_paramType.find(key);

	if (paramType_it == m_paramType.end())
		throw std::runtime_error("Trying to set an unknown key: " + std::string(inKey));

	switch(paramType_it->second)
	{
		case INT: set<int>(key, TU::strToInt(value)); break;
		case FLOAT: set<double>(key, TU::strToDouble(value)); break;
		case BOOL: set<bool>(key, validTrue.contains(value)); break;
		case STRING: set(key, value); break;
		case LIST_INT: set<std::vector<int>>(key, parseIntList(value)); break;
		case LIST_FLOAT: set<std::vector<double>>(key, parseFloatList(value)); break;
	}
}

std::string ParamStorage::printCurrentValues() const
{
  std::stringstream out;
  out << "---------------------\n";
	for (const auto &[key, valueList] : m_paramQueues)
		out << key << ": " << toString(valueList.back()) << "\n";
  return out.str();
}

std::string ParamStorage::printAllValues() const
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

void ParamStorage::pop(const std::string &anykey)
{
	std::string key = tolower(anykey);
	m_paramQueues[key].pop_back();
}

void ParamStorage::pop(const std::vector<std::string>& listOfParams)
{
    for (const auto& p: listOfParams){
        pop(p);
    }
}

}//end namespace TU
