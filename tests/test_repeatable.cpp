#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "tubul.h"

TEST(TUBULArgs, testRepeatable) {
    // We simulate the arguments that would come from the command line
    std::vector<const char*> argv = {
        "test_repeatable",
        "-t", "P1=V1",
        "-t", "P2=V2",
        "-v", "-v",
        "-l", "L1", "L2",
        "-l", "L3"
    };
    int argc = static_cast<int>(argv.size());

    // Register arguments
    // Note: Since the parser is global and static, this works best if 
    // no other test has already initialized the parser with conflicting names.
    TU::addArgument("-t", "--parameter_overload").setAsRepeatable();
    TU::addArgument("-v", "--verbose").setAsFlag().setAsRepeatable();
    TU::addArgument("-l", "--list").setAsList().setAsRepeatable();

    // Parse the simulated arguments
    // We use a try-catch because parseArgsOrDie might call exit() on failure 
    // in some implementations, but here we want to verify it works.
    TU::parseArgsOrDie(argc, argv.data());

    // Verify -t (Repeatable Strings)
    auto overloads = TU::getArg<std::vector<std::string>>("-t");
    EXPECT_EQ(overloads.size(), 2);
    EXPECT_EQ(overloads[0], "P1=V1");
    EXPECT_EQ(overloads[1], "P2=V2");

    // Verify -v (Repeatable Flags)
    auto verbosity = TU::getArg<std::vector<bool>>("-v");
    EXPECT_EQ(verbosity.size(), 2);

    // Verify -l (Repeatable Lists)
    auto lists = TU::getArg<std::vector<std::string>>("-l");
    EXPECT_EQ(lists.size(), 3);
    EXPECT_EQ(lists[0], "L1");
    EXPECT_EQ(lists[1], "L2");
    EXPECT_EQ(lists[2], "L3");
}
