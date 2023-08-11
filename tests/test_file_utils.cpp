//
// Created by Carlos Acosta on 30-03-23.
//

#include "tubul.h"
#include <gtest/gtest.h>
#include <fstream>
#include <iterator>

const char* FILE_TO_CREATE_CONTENTS = R"(This is the first line
and this is the second
some numbers here
12
13
14
and a goodbye
)";
const char* TEST_FILENAME = "test_file.txt";


TEST(TUBULFileUtils, testParseInt) {
    const char* val1 = "452";

    auto parsed1 = TU::strToInt(val1);
    EXPECT_EQ(parsed1, 452);

    std::string strval1("888");
    auto parsed2 = TU::strToInt(strval1);
    EXPECT_EQ(parsed2, 888);

    std::string_view strview(strval1);
    auto parsed3 = TU::strToInt(strview);
    EXPECT_EQ(parsed3, 888);

    EXPECT_EQ(TU::strToInt("-15"), -15);
    EXPECT_EQ(TU::strToInt("0"), 0);
    EXPECT_EQ(TU::strToInt("000000"), 0);
    EXPECT_EQ(TU::strToInt("000000"), 0);
    EXPECT_EQ(TU::strToInt("2000000000"), 2000000000);
    EXPECT_EQ(TU::strToInt("-2000000000"), -2000000000);

    //should fail because these strings have weird chars
    EXPECT_ANY_THROW( auto fail = TU::strToInt("3.14"));
    EXPECT_ANY_THROW( auto fail = TU::strToInt("314#"));
    EXPECT_ANY_THROW( auto fail = TU::strToInt(" 314#"));
    EXPECT_ANY_THROW( auto fail = TU::strToInt("31/3"));
}

TEST(TUBULFileUtils, testParseDouble) {
    const char* val1 = "452";

    auto parsed1 = TU::strToDouble(val1);
    EXPECT_EQ(parsed1, 452);

    std::string strval1("888.8");
    auto parsed2 = TU::strToDouble(strval1);
    EXPECT_EQ(parsed2, 888.8);

    std::string_view strview(strval1);
    auto parsed3 = TU::strToDouble(strview);
    EXPECT_EQ(parsed3, 888.8);


    EXPECT_EQ(TU::strToDouble("-15"), -15);
    EXPECT_EQ(TU::strToDouble("0.0"), 0);
    EXPECT_EQ(TU::strToDouble("0.5"), 0.5);
    EXPECT_EQ(TU::strToDouble("000000"), 0);
    EXPECT_EQ(TU::strToDouble("-0"), 0);
    EXPECT_EQ(TU::strToDouble("2000000000"), 2000000000);
    EXPECT_EQ(TU::strToDouble("-2000000000"), -2000000000);
    EXPECT_EQ(TU::strToDouble("3.14"), 3.14);
    EXPECT_EQ(TU::strToDouble("-3.14"), -3.14);
    EXPECT_EQ(TU::strToDouble("1.2e-5"), 1.2e-5);
    EXPECT_EQ(TU::strToDouble("-1e-9"), -1e-9);


    //should fail because these strings have weird chars
    EXPECT_ANY_THROW( auto fail = TU::strToDouble("314#"));
    EXPECT_ANY_THROW( auto fail = TU::strToDouble(" 314#@"));
    EXPECT_ANY_THROW( auto fail = TU::strToDouble("31/3"));
}

void createFile(){
    std::ofstream testFile(TEST_FILENAME, std::ios::binary);
    testFile.write(FILE_TO_CREATE_CONTENTS, strnlen(FILE_TO_CREATE_CONTENTS, 2048));
    testFile.close();
}

TEST(TUBULFileUtils, testReadWithLineIterator){
    createFile();
    std::ifstream input(TEST_FILENAME);
    auto begin = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();

    std::string file_contents( begin, end );
    EXPECT_EQ(file_contents, FILE_TO_CREATE_CONTENTS);

    {///From a string
        std::stringstream resultByLine;
        for (const auto &line: TU::slinerange(file_contents)) {
            resultByLine << line << "\n";
        }
        EXPECT_EQ(resultByLine.str(), FILE_TO_CREATE_CONTENTS);
    }
    {///From a string_view
        std::string_view sview = file_contents;
        std::stringstream resultByLine;
        for (const auto &line: TU::slinerange(sview)) {
            resultByLine << line << "\n";
        }
        EXPECT_EQ(resultByLine.str(), FILE_TO_CREATE_CONTENTS);
    }


}

TEST(TUBULFileUtils, testMMapFile){
    createFile();

    TU::MappedFile input(TEST_FILENAME);
    auto file_contents = input.string_view();
    EXPECT_EQ(file_contents, FILE_TO_CREATE_CONTENTS);


    {///From a string
        std::string copied_string(file_contents.data(), file_contents.size());
        std::stringstream resultByLine;
        for (const auto &line: TU::slinerange(copied_string)) {
            resultByLine << line << "\n";
        }
        EXPECT_EQ(resultByLine.str(), FILE_TO_CREATE_CONTENTS);
    }
    {///From a string_view
        std::string_view sview = file_contents;
        std::stringstream resultByLine;
        for (const auto &line: TU::slinerange(sview)) {
            resultByLine << line << "\n";
        }
        EXPECT_EQ(resultByLine.str(), FILE_TO_CREATE_CONTENTS);
    }


}