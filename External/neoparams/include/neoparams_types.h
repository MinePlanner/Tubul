//
// Created by Carlos Acosta on 06-11-23.
//

#pragma once
#include <variant>
#include <string>
#include "json.hpp"

namespace NeoParams {

    enum NeoParamsType {
        INT,
        FLOAT,
        STRING,
        BOOL,
        LIST_INT,
        LIST_FLOAT
    };
    // possible values to store on neoparams engine
    using ParamValue = std::variant<int, double, bool, std::string, std::vector<int>, std::vector<double>>;


    // utils
    ParamValue getParamDefDefault(nlohmann::json &paramdef, NeoParamsType paramsType);

    NeoParamsType getParamDefType(nlohmann::json &paramdef);

    std::string tolower(std::string const &word);

    std::string toString(ParamValue const &pv);

}