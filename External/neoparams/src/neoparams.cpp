//
// Created by Nicolas Loira on 5/22/21.
//

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <regex>
#include <exception>

#include "neoparams.h"
#include "paramdef.h"

//////////////////////////////////////////////////////////////////////
// NeoParams
//////////////////////////////////////////////////////////////////////
namespace NeoParams{


namespace {
    std::unique_ptr<NeoParamsData> global_instance = nullptr;
}

void init(std::istream& input)
{
    global_instance = std::make_unique<NeoParamsData>(input);
}

void init(const std::string& input)
{
    global_instance = std::make_unique<NeoParamsData>(input);
}

NeoParamsData& getInstance()
{
    if (global_instance != nullptr)
        return *global_instance;

    throw;
}

NeoParamsData::NeoParamsData(std::istream& input):
        m_paramDef(nlohmann::json::parse(input)){
    startup();
}

NeoParamsData::NeoParamsData(const std::string& input):
        m_paramDef(nlohmann::json::parse(input)){
    startup();
}

void NeoParamsData::startup()
{
    if (m_paramDef.empty())
        throw;

    for (auto &[section, paramdefs] : m_paramDef.items()) {
        for (auto &paramdef : paramdefs) {
            std::string full_name = tolower(section) + "." + tolower(paramdef["name"].get<std::string>());

            NeoParamsType paramType = getParamDefType(paramdef);
            m_paramType[full_name] = paramType;
            m_paramQueues[full_name].push_back(getParamDefDefault(paramdef, paramType));
        }
    }
}


void loadFromFile(const std::string& paramFile)
{
	NeoParamsData &params = NeoParams::getInstance();

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
    NeoParamsData &params = NeoParams::getInstance();

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

void NeoParamsData::setFromString(std::string const &key, std::string const &value)
{
	// try to convert value to the type defined on paramdef
	std::set<std::string> validTrue = {"yes", "true", "True", "1"};

	if (m_paramType.count(key) == 0)
		throw std::runtime_error("Trying to set an unknown key: "+key);

	NeoParamsType paramType = m_paramType[key];

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




std::string NeoParamsData::printCurrentValues() {

    std::stringstream out;
	out << "---------------------\n";
	for (const auto &[key, valueList] : m_paramQueues)
		out << key << ": " << toString(valueList.back()) << "\n";

    return out.str();
}

std::string NeoParamsData::printAllValues() {

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


void NeoParamsData::pop(const std::string &anykey)
{
	std::string key = tolower(anykey);
	m_paramQueues[key].pop_back();
}

} //end namespace neoparams
