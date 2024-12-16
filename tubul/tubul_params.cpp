//
// Created by Carlos Acosta on 30-06-23.
//

#include <vector>
#include "tubul_params.h"
#include "tubul_params_impl.h"
namespace TU
{

void configParams(std::istream& paramsConfig) {
    Parameters::init(paramsConfig);
}

void configParams(const std::string& paramsConfig) {
    Parameters::init(paramsConfig);
}


// load a params file (as many times as you want)
void loadParams(const std::string& paramsFile){
    Parameters::loadFromFile(paramsFile);
}

// load a params string (as many times as you want)
void loadParamsString(const std::string& content){
    Parameters::loadFromString(content);
}

// return to the previous value of a param
void popParam(const std::string& param){
    Parameters::pop(param);
}

// return several params to their previous values
void popParams(const std::vector<std::string>& listOfParams){
    for (const auto& p: listOfParams){
        Parameters::pop(p);
    }
}

// get the current value of a param
template <> int getParam(const std::string& param) { return Parameters::get<int>(param); }
template <> double getParam(const std::string& param) { return Parameters::get<double>(param); }
template <> bool getParam(const std::string& param) { return Parameters::get<bool>(param); }
template <> std::string getParam(const std::string& param) { return Parameters::get<std::string>(param); }
template <> std::vector<int> getParam(const std::string& param) { return Parameters::get<std::vector<int>>(param); }
template <> std::vector<double> getParam(const std::string& param) { return Parameters::get<std::vector<double>>(param); }

//Set the param to a new value
void setParam(const std::string& param, int value) { Parameters::set<int>( param, value); }
void setParam(const std::string& param, double value) { Parameters::set<double>( param, value); }
void setParam(const std::string& param, bool value) { Parameters::set<bool>( param, value); }
void setParam(const std::string& param, const std::string& value) { Parameters::set<std::string>( param, value); }
void setParam(const std::string& param, const char* value) { Parameters::set<std::string>( param, std::string(value) ); }
void setParam(const std::string& param, const std::vector<int>& value) { Parameters::set<std::vector<int>>( param, value); }
void setParam(const std::string& param, const std::vector<double>& value) { Parameters::set<std::vector<double>>( param, value); }

//Set the param to a temporary new value
void pushParam(const std::string& param, int value) { Parameters::push(param,value); }
void pushParam(const std::string& param, double value) { Parameters::push(param,value); }
void pushParam(const std::string& param, bool value) { Parameters::push(param,value); }
void pushParam(const std::string& param, const char* value) { Parameters::push(param,std::string(value)); }
void pushParam(const std::string& param, const std::string& value) { Parameters::push(param,value); }
void pushParam(const std::string& param, const std::vector<int>& value) { Parameters::push(param,value); }
void pushParam(const std::string& param, const std::vector<double>& value) { Parameters::push(param,value); }


void dumpParams(std::ostream& out){
    out << Parameters::dumpParams();
}
void dumpAllParams(std::ostream& out){
    out << Parameters::dumpAllParams();
}

}//end namespace TU