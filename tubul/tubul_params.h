//
// Created by Carlos Acosta on 30-06-23.
//

#pragma once

#include <string>
#include <iosfwd>

namespace TU
{
    // Reset any parameter configuration
    void clearParams();

    // Passes the configuration of parameters to parse.
    void configParams(std::istream& paramsConfig);
    void configParams(const std::string& paramsConfig);

    // load a params file (as many times as you want)
    void loadParams(const std::string&  paramsFile);

    // load a params file (as many times as you want)
    void loadParamsString(const std::string& content);

    // get the current value of a param
    template <typename T>
    T getParam(const std::string& param);

    // set the current value of a param
    void setParam(const std::string& param, int value) ;
    void setParam(const std::string& param, double value) ;
    void setParam(const std::string& param, bool value) ;
    void setParam(const std::string& param, const char* value) ;
    void setParam(const std::string& param, const std::string& value) ;
    void setParam(const std::string& param, const std::vector<int>& value) ;
    void setParam(const std::string& param, const std::vector<double>& value) ;

    // temporarily changes the value of a param
    void pushParam(const std::string& param, int value) ;
    void pushParam(const std::string& param, double value) ;
    void pushParam(const std::string& param, bool value) ;
    void pushParam(const std::string& param, const char* value) ;
    void pushParam(const std::string& param, const std::string& value) ;
    void pushParam(const std::string& param, const std::vector<int>& value) ;
    void pushParam(const std::string& param, const std::vector<double>& value) ;

    // return to the previous value of a param
    void popParam(const std::string& param);

    // return several params to their previous values
    void popParams(const std::vector<std::string>& listOfParams);

    // return a copy of a section, where keys are at the root level
    // (should be recovered with "key", not with "section.key")
//    Parameters getSection(std::string_view& section);

    // get a copy All parameters
//    Parameters &getAllParameters();

    // get parameters that differs from default
//    Parameters &getNonDefaultParameters();

    // dump all parameters
    void dumpAllParams(std::ostream& out);
    void dumpParams(std::ostream& out);



} //end namespace TU