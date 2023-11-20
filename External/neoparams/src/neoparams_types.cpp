//
// Created by Carlos Acosta on 06-11-23.
//

#include <sstream>
#include <unordered_map>
#include "neoparams_types.h"

namespace NeoParams
{

// utils
ParamValue getParamDefDefault(nlohmann::json &paramdef, NeoParamsType paramsType)

{
    ParamValue data;

    switch (paramsType) {
        case NeoParamsType::INT: data = paramdef.contains("default") ? paramdef["default"].get<int>() : 0; break;
        case NeoParamsType::FLOAT: data = paramdef.contains("default") ? paramdef["default"].get<double>() : 0.0; break;
        case NeoParamsType::BOOL: data = paramdef.contains("default") && paramdef["default"].get<bool>(); break;
        case NeoParamsType::STRING: data = paramdef.contains("default") ? paramdef["default"].get<std::string>() : std::string(""); break;
        case NeoParamsType::LIST_INT: data = paramdef.contains("default") ? paramdef["default"].get<std::vector<int>>() : std::vector<int>(); break;
        case NeoParamsType::LIST_FLOAT: data = paramdef.contains("default") ? paramdef["default"].get<std::vector<double>>() : std::vector<double>(); break;
    }

    return data;
}

NeoParamsType getParamDefType(nlohmann::json &paramdef)
{
    static const std::unordered_map<std::string, NeoParamsType> stringToType = {
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

std::string tolower(std::string const &word)
{
    std::string data(word);
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return data;
}

struct ParamValuePrinter{
    template<typename T>
    std::string simpleToString(T pv) const { return std::to_string(pv);}

    template<typename T>
    std::string vectorToString(const std::vector<T>& pv) const{
        if (pv.empty()) return "[ ]";
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

} //end namespace NeoParams