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
    //Configure params
    TU::addParamsConfig( TEST_PARAMDEF );

	{
		//Check the default values.
    	EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
    	EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
    	EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
    	EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
    	std::vector<int> intExpected = {1,1,1,2,2,4,8};
    	EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), intExpected);
    	std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
    	EXPECT_EQ( TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
	}

	{
		//Redundant definitions are allowed
    	TU::addParamsConfig( TEST_PARAMDEF );
    	EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
    	EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
    	EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
    	EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
    	std::vector<int> intExpected = {1,1,1,2,2,4,8};
    	EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), intExpected);
    	std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
    	EXPECT_EQ( TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
	}

	TU::clearParams();
}

TEST(TUBULParams, settingParams) {
    //Configure params
    TU::addParamsConfig( TEST_PARAMDEF );

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
    //Int list
    std::vector<int> intExpectedDefault = {1,1,1,2,2,4,8};
    std::vector<int> IntExpectedNew = {1,1,2,3,5,8,13};
    paramUpdateChecker("Zoo.MagicSequence", intExpectedDefault, IntExpectedNew );
    //Float list
    std::vector<double> floatExpectedDefault = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
    std::vector<double> floatExpectedNew = {1.12,1.031,2.56,3.1,5.907,8.2,13.34};
    paramUpdateChecker("Zoo.FloatMagicSequence", floatExpectedDefault, floatExpectedNew);

	TU::clearParams();
}


TEST(TUBULParams, pushPopSingleParam) {
    //Configure params
    TU::addParamsConfig( TEST_PARAMDEF );

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
    //Int list
    std::vector<int> intExpectedDefault = {1,1,1,2,2,4,8};
    std::vector<int> IntExpectedNew = {1,1,2,3,5,8,13};
    paramPushPopChecker("Zoo.MagicSequence", intExpectedDefault, IntExpectedNew );
    //Float list
    std::vector<double> floatExpectedDefault = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
    std::vector<double> floatExpectedNew = {1.12,1.031,2.56,3.1,5.907,8.2,13.34};
    paramPushPopChecker("Zoo.FloatMagicSequence", floatExpectedDefault, floatExpectedNew);

	TU::clearParams();
}

TEST(TUBULParams, pushSingleParamPopSeveral) {
    //Configure params
    TU::addParamsConfig( TEST_PARAMDEF );

    //Check the default values.
    {
        EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
        EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
        EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
        EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
        std::vector<int> intExpected = {1,1,1,2,2,4,8};
        EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), intExpected);
        std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
        EXPECT_EQ( TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
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
        std::vector<int> newIntSeq = {2,4,6,8,10};
        TU::pushParam("Zoo.MagicSequence", newIntSeq);
        std::vector<double> newFloatSeq = {2.1,4.12,6.123,8.1234,10.12345};
        TU::pushParam("Zoo.FloatMagicSequence", newFloatSeq);
    }
    //Check the pushed values just in case
    {
        EXPECT_EQ(TU::getParam<int>("Global.timeout"), 42);
        EXPECT_EQ(TU::getParam<std::string>("Global.solver"), "juanin");
        EXPECT_EQ(TU::getParam<double>("Zoo.MagicValue"), 1.6180);
        EXPECT_EQ(TU::getParam<bool>("Zoo.LionShowAvailable"), false);
        std::vector<int> newIntSeq= {2,4,6,8,10};
        EXPECT_EQ(TU::getParam<std::vector<int>>("Zoo.MagicSequence"), newIntSeq);
        std::vector<double> newFloatSeq = {2.1,4.12,6.123,8.1234,10.12345};
        EXPECT_EQ(TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), newFloatSeq);
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
                                          "Zoo.MagicSequence",
                                          "Zoo.FloatMagicSequence"};
    TU::popParams(keysToPop);
    //Check the params returned to normal
    {
        EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
        EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
        EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
        EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
        std::vector<int> intExpected = {1,1,1,2,2,4,8};
        EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), intExpected);
        std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
        EXPECT_EQ( TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
    }

	TU::clearParams();
}

TEST(TUBULParams, readFromIniFile) {
    //Configure params
    std::string iniFileName = "test_file.ini";
    TU::addParamsConfig( TEST_PARAMDEF );
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
    std::vector<int> intExpected = {30,10,50,120,70};
    EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), intExpected);
    std::vector<double> floatExpected = {30.31,10.21,50.46,120.87,70.13}; 
    EXPECT_EQ( TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);

	TU::clearParams();
}

TEST(TUBULParams, readFromIniString) {
    TU::addParamsConfig( TEST_PARAMDEF );
    //Loading ini data from string that contains several keys.
    TU::loadParamsString(TEST_INIFILE);

    //The keys should be what the file said.
    EXPECT_EQ( TU::getParam<int>("Global.timeout"), 21 );
    EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "Gurobi" );
    EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 420 );
    EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), false );
    std::vector<int> intExpected = {30,10,50,120,70};
    EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), intExpected);
    std::vector<double> floatExpected = {30.31,10.21,50.46,120.87,70.13};
    EXPECT_EQ( TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);

	TU::clearParams();
}

TEST(TUBULParams, usingDefault)
{
	TU::addParamsConfig( TEST_PARAMDEF );

	// all params should be using their defaults values initially
	EXPECT_EQ( TU::usingDefault("Global.timeout"), true );
	EXPECT_EQ( TU::usingDefault("Global.solver"), true );
	EXPECT_EQ( TU::usingDefault("Zoo.LionShowAvailable"), true );
	EXPECT_EQ( TU::usingDefault("Zoo.MagicValue"), true );
	EXPECT_EQ( TU::usingDefault("Zoo.MagicSequence"), true);
	EXPECT_EQ( TU::usingDefault("Zoo.FloatMagicSequence"), true);

	TU::setParam("Global.timeout", 2);
	TU::setParam("Global.solver", "gurobi");
	TU::setParam("Zoo.LionShowAvailable", false);
	TU::setParam("Zoo.MagicValue", 3.14);
	std::vector<int> newInt = {1, 2, 3, 4, 5};
	TU::setParam("Zoo.MagicSequence", newInt);
	std::vector<double> newFloat = {0.2, 0.33, 0.56, 0.82, 0.97};
	TU::setParam("Zoo.FloatMagicSequence", newFloat);

	// the default-defined value of a params doesnt change with the parameter
	EXPECT_EQ( TU::getDefault<int>("Global.timeout"), 0);
	EXPECT_EQ( TU::getDefault<std::string>("Global.solver"), "cplex" );
	EXPECT_EQ( TU::getDefault<bool>("Zoo.LionShowAvailable"), true );
	EXPECT_EQ( TU::getDefault<double>("Zoo.MagicValue"), 3.1415 );
	std::vector<int> intExpected = {1,1,1,2,2,4,8};
	EXPECT_EQ( TU::getDefault<std::vector<int>>("Zoo.MagicSequence"), intExpected );
	std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
	EXPECT_EQ( TU::getDefault<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected );

	TU::clearParams();
}

TEST(TUBULParams, multipleConfigs)
{
	TU::addParamsConfig( TEST_PARAMDEF );
	TU::addParamsConfig( OTHER_TEST_PARAMDEF);

	{
		//Check the default values of the second params
		EXPECT_EQ( TU::getParam<int>("Global.retries"), 5 );
		EXPECT_EQ( TU::getParam<std::string>("Global.format"), "CSV" );
		EXPECT_EQ( TU::getParam<bool>("Time.ran_out"), false );
		EXPECT_EQ( TU::getParam<double>("Time.elapsed_time"), 0.0 );
		std::vector<int> intExpected = {101,102,103,104,105};
		EXPECT_EQ( TU::getParam<std::vector<int>>("Time.events"), intExpected);
		std::vector<double> floatExpected = {0.35, 5.85, 12.6, 200.6, 30.2};
		EXPECT_EQ( TU::getParam<std::vector<double>>("Time.events_times"), floatExpected);
	}

	{
		//Check that the other values are still there.
		EXPECT_EQ( TU::getParam<int>("Global.timeout"), 0 );
		EXPECT_EQ( TU::getParam<std::string>("Global.solver"), "cplex" );
		EXPECT_EQ( TU::getParam<double>("Zoo.MagicValue"), 3.1415 );
		EXPECT_EQ( TU::getParam<bool>("Zoo.LionShowAvailable"), true );
		std::vector<int> intExpected = {1,1,1,2,2,4,8};
		EXPECT_EQ( TU::getParam<std::vector<int>>("Zoo.MagicSequence"), intExpected);
		std::vector<double> floatExpected = {1.23,1.45,1.67,2.122,2.09,4.67,8.8901};
		EXPECT_EQ( TU::getParam<std::vector<double>>("Zoo.FloatMagicSequence"), floatExpected);
	}

	TU::clearParams();
}
