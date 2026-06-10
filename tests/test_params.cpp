//
// Created by Carlos Acosta on 06-11-23.
//

#include <fstream>
#include "tubul.h"
#include <gtest/gtest.h>


const char *TEST_PARAMDEF = R"###(
{
"Global": [
    {
        "name": "timeout",
        "description": "max number of seconds for full execution. 0=unlimited"
    },
    {
        "name": "solver",
        "description": "solver engine to be used. Requires support at compilation time",
        "type": "str",
        "values": ["cplex", "gurobi", "coin"],
        "default": "cplex"
    }
    ],
"Zoo": [
    {
        "name": "LionShowAvailable",
        "description": "Signals if we can watch the lion's show",
        "type": "bool",
        "default": true
    },
    {
        "name": "MagicValue",
        "description": "magical value we can provide for the solver",
        "type": "float",
        "default": 3.1415
    },
    {
        "name":"MagicSequence",
        "description": "Magical sequence used in the processing",
        "type": "list_int",
        "default": [1,1,1,2,2,4,8]
    },
    {
        "name" : "FloatMagicSequence",
        "description" : "Magical sequence, but of float type",
        "type" : "list_float",
        "default" : [1.23,1.45,1.67,2.122,2.09,4.67,8.8901]
    }
    ]
}
)###";


const char *OTHER_TEST_PARAMDEF = R"###(
{
"Global": [
    {
        "name": "retries",
        "description": "max number of seconds for a rerty after a timeout. 0=unlimited",
		"type" : "int",
		"default" : 5
    },
	{
		"name" : "format",
		"description" : "desired format for the output",
		"type" : "str",
		"default" : "CSV"
	}
    ],
"Time": [
    {
        "name": "ran_out",
        "description": "Signals if the timer's time has passed",
        "type": "bool",
        "default": false
    },
    {
        "name": "elapsed_time",
        "description": "defines the elapsed time since a certain point in time",
		"type" : "float"
    },
    {
        "name": "events",
        "description": "timed events id's",
        "type": "list_int",
        "default": [101, 102, 103, 104, 105]
    },
    {
        "name" : "events_times",
        "description" : "expected times for the events execution",
        "type" : "list_float",
        "default" : [0.35, 5.85, 12.6, 200.6, 30.2]
    }
    ]
}
)###";

const char* TEST_INIFILE = R"###(
[Global]
; A comment
timeout = 21
solver= Gurobi ; another comment
[Zoo]
LionShowAvailable =false
MagicValue=420 # and a third for good measure
MagicSequence = [30,10,50,120,70]
FloatMagicSequence = [30.31,10.21,50.46,120.87,70.13]
)###";

TEST(TUBULParams, definitions) {
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);

    {
        EXPECT_EQ(params.get<int>("Global.timeout"), 0);
        EXPECT_EQ(params.get<std::string>("Global.solver"), "cplex");
        EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 3.1415);
        EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), true);
        std::vector<int> intExpected = {1,1,1,2,2,4,8};
        EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), intExpected);
        std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
        EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
    }

    {
        //Redundant definitions are allowed
        params.defineParams(TEST_PARAMDEF);
        EXPECT_EQ(params.get<int>("Global.timeout"), 0);
        EXPECT_EQ(params.get<std::string>("Global.solver"), "cplex");
        EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 3.1415);
        EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), true);
        std::vector<int> intExpected = {1,1,1,2,2,4,8};
        EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), intExpected);
        std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
        EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
    }
}

TEST(TUBULParams, helpTexts) {
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);

    const auto& helpTexts = params.getHelpTexts();
    EXPECT_EQ(helpTexts.size(), 6);
    EXPECT_EQ(helpTexts.at("global.timeout"), "max number of seconds for full execution. 0=unlimited");
    EXPECT_EQ(helpTexts.at("zoo.lionshowavailable"), "Signals if we can watch the lion's show");

    //A param without description gets an empty help text
    params.defineParams(R"###({"Extra": [{"name": "undocumented"}]})###");
    EXPECT_EQ(params.getHelpTexts().at("extra.undocumented"), "");

    //clear() also clears the help texts
    params.clear();
    EXPECT_TRUE(params.getHelpTexts().empty());
}

TEST(TUBULParams, settingParams) {
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);

    auto paramUpdateChecker = [&](const std::string& name, auto oldValue, auto newValue){
        using paramType = decltype(oldValue);
        EXPECT_EQ(params.get<paramType>(name), oldValue);
        params.set(name, newValue);
        EXPECT_NE(params.get<paramType>(name), oldValue);
        EXPECT_EQ(params.get<paramType>(name), newValue);
    };

    paramUpdateChecker("Global.timeout", 0, 42);
    paramUpdateChecker("Zoo.MagicValue", 3.1415, 2.7182818);
    paramUpdateChecker("Zoo.LionShowAvailable", true, false);
    paramUpdateChecker("Global.solver", std::string("cplex"), std::string("abacus"));
    std::vector<int> intExpectedDefault = {1,1,1,2,2,4,8};
    std::vector<int> intExpectedNew = {1,1,2,3,5,8,13};
    paramUpdateChecker("Zoo.MagicSequence", intExpectedDefault, intExpectedNew);
    std::vector<double> floatExpectedDefault = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
    std::vector<double> floatExpectedNew = {1.12,1.031,2.56,3.1,5.907,8.2,13.34};
    paramUpdateChecker("Zoo.FloatMagicSequence", floatExpectedDefault, floatExpectedNew);
}

TEST(TUBULParams, pushPopSingleParam) {
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);

    auto paramPushPopChecker = [&](const std::string& name, auto oldValue, auto newValue){
        using paramType = decltype(oldValue);
        EXPECT_EQ(params.get<paramType>(name), oldValue);
        params.push(name, newValue);
        EXPECT_NE(params.get<paramType>(name), oldValue);
        EXPECT_EQ(params.get<paramType>(name), newValue);
        params.pop(name);
        EXPECT_EQ(params.get<paramType>(name), oldValue);
    };

    paramPushPopChecker("Global.timeout", 0, 42);
    paramPushPopChecker("Zoo.MagicValue", 3.1415, 2.7182818);
    paramPushPopChecker("Zoo.LionShowAvailable", true, false);
    paramPushPopChecker("Global.solver", std::string("cplex"), std::string("abacus"));
    std::vector<int> intExpectedDefault = {1,1,1,2,2,4,8};
    std::vector<int> intExpectedNew = {1,1,2,3,5,8,13};
    paramPushPopChecker("Zoo.MagicSequence", intExpectedDefault, intExpectedNew);
    std::vector<double> floatExpectedDefault = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
    std::vector<double> floatExpectedNew = {1.12,1.031,2.56,3.1,5.907,8.2,13.34};
    paramPushPopChecker("Zoo.FloatMagicSequence", floatExpectedDefault, floatExpectedNew);
}

TEST(TUBULParams, pushSingleParamPopSeveral) {
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);

    {
        EXPECT_EQ(params.get<int>("Global.timeout"), 0);
        EXPECT_EQ(params.get<std::string>("Global.solver"), "cplex");
        EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 3.1415);
        EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), true);
        std::vector<int> intExpected = {1,1,1,2,2,4,8};
        EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), intExpected);
        std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
        EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
    }
    {
        std::stringstream dump;
        dump << params.printCurrentValues();
        std::cout << dump.str();
    }
    {
        params.push("Global.timeout", 42);
        params.push("Global.solver", "juanin");
        params.push("Zoo.MagicValue", 1.6180);
        params.push("Zoo.LionShowAvailable", false);
        std::vector<int> newIntSeq = {2,4,6,8,10};
        params.push("Zoo.MagicSequence", newIntSeq);
        std::vector<double> newFloatSeq = {2.1,4.12,6.123,8.1234,10.12345};
        params.push("Zoo.FloatMagicSequence", newFloatSeq);
    }
    {
        EXPECT_EQ(params.get<int>("Global.timeout"), 42);
        EXPECT_EQ(params.get<std::string>("Global.solver"), "juanin");
        EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 1.6180);
        EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), false);
        std::vector<int> newIntSeq = {2,4,6,8,10};
        EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), newIntSeq);
        std::vector<double> newFloatSeq = {2.1,4.12,6.123,8.1234,10.12345};
        EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), newFloatSeq);
    }
    {
        std::stringstream dump;
        dump << params.printCurrentValues();
        dump << params.printAllValues();
        std::cout << dump.str();
    }
    std::vector<std::string> keysToPop = {"Global.timeout",
                                          "Global.solver",
                                          "Zoo.MagicValue",
                                          "Zoo.LionShowAvailable",
                                          "Zoo.MagicSequence",
                                          "Zoo.FloatMagicSequence"};
    params.pop(keysToPop);
    {
        EXPECT_EQ(params.get<int>("Global.timeout"), 0);
        EXPECT_EQ(params.get<std::string>("Global.solver"), "cplex");
        EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 3.1415);
        EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), true);
        std::vector<int> intExpected = {1,1,1,2,2,4,8};
        EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), intExpected);
        std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
        EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
    }
}

TEST(TUBULParams, readFromIniFile) {
    TU::ParamStorage params;
    std::string iniFileName = "test_file.ini";
    params.defineParams(TEST_PARAMDEF);
    {
        std::ofstream iniFile(iniFileName);
        iniFile << TEST_INIFILE;
    }

    params.loadParamsFile(iniFileName);

    EXPECT_EQ(params.get<int>("Global.timeout"), 21);
    EXPECT_EQ(params.get<std::string>("Global.solver"), "Gurobi");
    EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 420);
    EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), false);
    std::vector<int> intExpected = {30,10,50,120,70};
    EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), intExpected);
    std::vector<double> floatExpected = {30.31,10.21,50.46,120.87,70.13};
    EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
}

TEST(TUBULParams, readFromIniString) {
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);
    params.loadParamsString(TEST_INIFILE);

    EXPECT_EQ(params.get<int>("Global.timeout"), 21);
    EXPECT_EQ(params.get<std::string>("Global.solver"), "Gurobi");
    EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 420);
    EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), false);
    std::vector<int> intExpected = {30,10,50,120,70};
    EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), intExpected);
    std::vector<double> floatExpected = {30.31,10.21,50.46,120.87,70.13};
    EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
}

TEST(TUBULParams, usingDefault)
{
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);

    EXPECT_EQ(params.usingDefault("Global.timeout"), true);
    EXPECT_EQ(params.usingDefault("Global.solver"), true);
    EXPECT_EQ(params.usingDefault("Zoo.LionShowAvailable"), true);
    EXPECT_EQ(params.usingDefault("Zoo.MagicValue"), true);
    EXPECT_EQ(params.usingDefault("Zoo.MagicSequence"), true);
    EXPECT_EQ(params.usingDefault("Zoo.FloatMagicSequence"), true);

    params.set("Global.timeout", 2);
    params.set("Global.solver", std::string("gurobi"));
    params.set("Zoo.LionShowAvailable", false);
    params.set("Zoo.MagicValue", 3.14);
    std::vector<int> newInt = {1, 2, 3, 4, 5};
    params.set("Zoo.MagicSequence", newInt);
    std::vector<double> newFloat = {0.2, 0.33, 0.56, 0.82, 0.97};
    params.set("Zoo.FloatMagicSequence", newFloat);

    EXPECT_EQ(params.getDefault<int>("Global.timeout"), 0);
    EXPECT_EQ(params.getDefault<std::string>("Global.solver"), "cplex");
    EXPECT_EQ(params.getDefault<bool>("Zoo.LionShowAvailable"), true);
    EXPECT_EQ(params.getDefault<double>("Zoo.MagicValue"), 3.1415);
    std::vector<int> intExpected = {1,1,1,2,2,4,8};
    EXPECT_EQ(params.getDefault<std::vector<int>>("Zoo.MagicSequence"), intExpected);
    std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
    EXPECT_EQ(params.getDefault<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
}

TEST(TUBULParams, multipleConfigs)
{
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);
    params.defineParams(OTHER_TEST_PARAMDEF);

    {
        EXPECT_EQ(params.get<int>("Global.retries"), 5);
        EXPECT_EQ(params.get<std::string>("Global.format"), "CSV");
        EXPECT_EQ(params.get<bool>("Time.ran_out"), false);
        EXPECT_EQ(params.get<double>("Time.elapsed_time"), 0.0);
        std::vector<int> intExpected = {101,102,103,104,105};
        EXPECT_EQ(params.get<std::vector<int>>("Time.events"), intExpected);
        std::vector<double> floatExpected = {0.35, 5.85, 12.6, 200.6, 30.2};
        EXPECT_EQ(params.get<std::vector<double>>("Time.events_times"), floatExpected);
    }

    {
        EXPECT_EQ(params.get<int>("Global.timeout"), 0);
        EXPECT_EQ(params.get<std::string>("Global.solver"), "cplex");
        EXPECT_EQ(params.get<double>("Zoo.MagicValue"), 3.1415);
        EXPECT_EQ(params.get<bool>("Zoo.LionShowAvailable"), true);
        std::vector<int> intExpected = {1,1,1,2,2,4,8};
        EXPECT_EQ(params.get<std::vector<int>>("Zoo.MagicSequence"), intExpected);
        std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
        EXPECT_EQ(params.get<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
    }
}

TEST(TUBULParams, stringViewKeys)
{
    TU::ParamStorage params;
    params.defineParams(TEST_PARAMDEF);

    //All key-taking methods should accept string_views directly.
    std::string_view timeoutKey("Global.timeout");
    std::string_view solverKey("Global.solver");

    EXPECT_EQ(params.get<int>(timeoutKey), 0);
    EXPECT_TRUE(params.usingDefault(timeoutKey));

    params.set(timeoutKey, 42);
    EXPECT_EQ(params.get<int>(timeoutKey), 42);
    EXPECT_FALSE(params.usingDefault(timeoutKey));
    EXPECT_EQ(params.getDefault<int>(timeoutKey), 0);

    //Setting a string param from a string_view value (uses the dedicated overload).
    std::string_view solverValue("gurobi");
    params.set(solverKey, solverValue);
    EXPECT_EQ(params.get<std::string>(solverKey), "gurobi");

    //String literals keep working and still land on the std::string alternative.
    params.set(solverKey, "coin");
    EXPECT_EQ(params.get<std::string>(solverKey), "coin");

    params.push(timeoutKey, 100);
    EXPECT_EQ(params.get<int>(timeoutKey), 100);
    params.pop("Global.timeout");
    EXPECT_EQ(params.get<int>(timeoutKey), 42);

    params.setFromString(std::string_view("global.timeout"), std::string_view("7"));
    EXPECT_EQ(params.get<int>(timeoutKey), 7);
}
