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
)###";

TEST(TUBULNeoParams, definitions) {
    //Configure params
    TU::configParams( TEST_PARAMDEF );

    //Check the default values.
    EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
    EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
    EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
    EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
    std::vector<int> expected = {1,1,1,2,2,4,8};
    EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), expected);


}

TEST(TUBULNeoParams, settingParams) {
    //Configure params
    TU::configParams( TEST_PARAMDEF );

    //Be careful of the types passed to old/new value!! I wrote this to avoid repeating
    //the same lines, but it can be misleading if you make an error regarding the types.
    auto paramUpdateChecker = [&](const std::string& name, auto oldValue, auto newValue){
        using paramType = decltype(oldValue);
        EXPECT_EQ( TU::getParam<paramType>(name), oldValue );
        TU::setParam(name, newValue);
        EXPECT_NE( TU::getParam<paramType>(name), oldValue );
        EXPECT_EQ( TU::getParam<paramType>(name), newValue);
    };

    //Integer setting and querying
    paramUpdateChecker("Global.timeout", 0, 42);
    //Floating point setting and querying
    paramUpdateChecker("Zoo.MagicValue", 3.1415, 2.7182818);
    //Boolean
    paramUpdateChecker("Zoo.LionShowAvailable", true, false);
    //String
    paramUpdateChecker("Global.solver",std::string("cplex"), std::string("abacus"));
    //Numbers list
    std::vector<int> expectedDefault = {1,1,1,2,2,4,8};
    std::vector<int> expectedNew = {1,1,2,3,5,8,13};
    paramUpdateChecker("Zoo.MagicSequence", expectedDefault, expectedNew );
}


TEST(TUBULNeoParams, pushPopSingleParam) {
    //Configure params
    TU::configParams( TEST_PARAMDEF );

    //Same concerns and lambda in previoous test
    auto paramPushPopChecker = [&](const std::string& name, auto oldValue, auto newValue){
        using paramType = decltype(oldValue);
        EXPECT_EQ( TU::getParam<paramType>(name), oldValue );
        TU::pushParam(name, newValue);
        EXPECT_NE( TU::getParam<paramType>(name), oldValue );
        EXPECT_EQ( TU::getParam<paramType>(name), newValue);
        TU::popParam(name);
        EXPECT_EQ( TU::getParam<paramType>(name), oldValue );
    };

    //Integer
    paramPushPopChecker("Global.timeout", 0, 42);
    //Floating point setting and querying
    paramPushPopChecker("Zoo.MagicValue", 3.1415, 2.7182818);
    //Boolean
    paramPushPopChecker("Zoo.LionShowAvailable", true, false);
    //String
    paramPushPopChecker("Global.solver",std::string("cplex"), std::string("abacus"));
    //Numbers list
    std::vector<int> expectedDefault = {1,1,1,2,2,4,8};
    std::vector<int> expectedNew = {1,1,2,3,5,8,13};
    paramPushPopChecker("Zoo.MagicSequence", expectedDefault, expectedNew );
}

TEST(TUBULNeoParams, pushSingleParamPopSeveral) {
    //Configure params
    TU::configParams( TEST_PARAMDEF );

    //Check the default values.
    {
        EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
        EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
        EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
        EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
        std::vector<int> expected = {1,1,1,2,2,4,8};
        EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), expected);
    }
    {
        std::stringstream dump;
        TU::dumpParams(dump);
        std::cout << dump.str();
    }
    //Push some values
    {
        TU::pushParam("Global.timeout", 42);
        TU::pushParam("Global.solver", "juanin");
        TU::pushParam("Zoo.MagicValue", 1.6180);
        TU::pushParam("Zoo.LionShowAvailable", false );
        std::vector<int> newSeq= {2,4,6,8,10};
        TU::pushParam("Zoo.MagicSequence", newSeq);
    }
    //Check the pushed values just in case
    {
        EXPECT_EQ(TU::getParam<int>("Global.timeout"), 42);
        EXPECT_EQ(TU::getParam<std::string>("Global.solver"), "juanin");
        EXPECT_EQ(TU::getParam<double>("Zoo.MagicValue"), 1.6180);
        EXPECT_EQ(TU::getParam<bool>("Zoo.LionShowAvailable"), false);
        std::vector<int> newSeq= {2,4,6,8,10};
        EXPECT_EQ(TU::getParam<std::vector<int>>("Zoo.MagicSequence"), newSeq);
    }
    {
        std::stringstream dump;
        TU::dumpParams(dump);
        TU::dumpAllParams(dump);
        std::cout << dump.str();
    }
    //Pop several params
    std::vector<std::string> keysToPop = {"Global.timeout",
                                          "Global.solver",
                                          "Zoo.MagicValue",
                                          "Zoo.LionShowAvailable",
                                          "Zoo.MagicSequence"};
    TU::popParams(keysToPop);
    //Check the params returned to normal
    {
        EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
        EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
        EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
        EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
        std::vector<int> expected = {1,1,1,2,2,4,8};
        EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), expected);
    }
}

TEST(TUBULNeoParams, readFromIni) {
    //Configure params
    std::string iniFileName = "test_file.ini";
    TU::configParams( TEST_PARAMDEF );
    {
        std::ofstream iniFile(iniFileName);
        iniFile << TEST_INIFILE;
    }

    //Loading ini file that contains several keys.
    TU::loadParams(iniFileName);

    //The keys should be what the file said.
    EXPECT_EQ( TU::getParam<int>("Global.timeout"), 21 );
    EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "Gurobi" );
    EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 420 );
    EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), false );
    std::vector<int> expected = { 30,10,50,120,70};
    EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), expected);

}
