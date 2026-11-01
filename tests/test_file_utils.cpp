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

const char* FILE_TO_CREATE_CONTENTSCRLF = "This is the first line\r\nand this is the second\r\nsome numbers here\r\n12\r\n13\r\n14\r\nand a goodbye\r\n";
const char* TEST_FILENAMECRLF = "test_file_crlf.txt";

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

void createFileCRLF(){
    std::ofstream testFile(TEST_FILENAMECRLF, std::ios::binary);
    testFile.write(FILE_TO_CREATE_CONTENTSCRLF, strnlen(FILE_TO_CREATE_CONTENTSCRLF, 2048));
    testFile.close();
}

TEST(TUBULFileUtils, testReadWithLineIteratorCRLF){
    createFileCRLF();

    std::ifstream input(TEST_FILENAMECRLF);
    auto begin = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();

    //string containing the "raw characters"
    std::string file_contents( begin, end );

    //Reading in a c++ stream-like way.
    std::stringstream toRead(file_contents);
    std::vector<std::string> toCompare;
    while (not toRead.eof()) {
        std::string stdline;
        std::getline(toRead, stdline);
        if ( stdline.empty() )
            break;
        if ( stdline.ends_with('\r') )
            stdline.resize( stdline.size() - 1);
        toCompare.emplace_back( stdline );
    }
    {///From a string
        std::vector<std::string> resultByLine;
        for (const auto &line: TU::slinerange(file_contents)) {
            resultByLine.emplace_back( line.data(), line.size() );
        }
        for (size_t i = 0; i < toCompare.size(); ++i) {
            EXPECT_EQ(toCompare[i], resultByLine[i]);
        }
    }
    {///From a string_view
        std::vector<std::string> resultByLine;
        std::string_view sview = file_contents;
        for (const auto &line: TU::slinerange(sview)) {
            resultByLine.emplace_back( line.data(), line.size() );
        }
        for (size_t i = 0; i < toCompare.size(); ++i) {
            EXPECT_EQ(toCompare[i], resultByLine[i]);
        }
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


TEST(TUBULFileUtils, testReadFileLines)
{
    createFile();
    
    {
    std::vector<std::string> expected = {
        "This is the first line",
        "and this is the second",
        "some numbers here",
        "12",
        "13",
        "14",
        "and a goodbye"
    };

    int idx = 0;
    TU::readFileLines(TEST_FILENAME, [&expected, &idx](const std::string_view &line){
        EXPECT_EQ(line, expected[idx]);
        idx++;
    });
    }

    {
    std::vector<std::string> expected = {
        "and this is the second",
        "12",
        "14",
    };

    int idx = 0, expected_idx = 0;
    TU::readFileLines(TEST_FILENAME, [&idx, &expected_idx, expected](const std::string_view &line){
        // ignores even indexed lines
        if (idx % 2 == 1)
        {
            EXPECT_EQ(line, expected[expected_idx]);
            expected_idx++;
        }
        idx++;
    });
    }

}


TEST(TUBULFileUtils, testOpenInputFilePlain)
{
    const std::string filename = "tubul_test_open_input_plain.txt";
    {
        std::ofstream out(filename);
        out << "first line\nsecond line\n";
    }
    std::string first, second;
    {
        const auto in = TU::openInputFile(filename);
        std::getline(*in, first);
        std::getline(*in, second);
    }
    std::remove(filename.c_str());
    EXPECT_EQ(first, "first line");
    EXPECT_EQ(second, "second line");
}

TEST(TUBULFileUtils, testOpenInputFileGzip)
{
    // "first line\nsecond line\n", compressed with gzip (Python: gzip.compress(..., mtime=0)).
    const unsigned char compressed[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x4b, 0xcb, 0x2c, 0x2a, 0x2e, 0x51,
        0xc8, 0xc9, 0xcc, 0x4b, 0xe5, 0x2a, 0x4e, 0x4d, 0xce, 0xcf, 0x4b, 0x81, 0xb0, 0x01, 0x2c, 0x5a,
        0x45, 0x5d, 0x17, 0x00, 0x00, 0x00
    };
    const std::string filename = "tubul_test_open_input.gz";
    {
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(compressed), sizeof(compressed));
    }
    std::string first, second;
    {
        const auto in = TU::openInputFile(filename);
        std::getline(*in, first);
        std::getline(*in, second);
    }
    std::remove(filename.c_str());
    EXPECT_EQ(first, "first line");
    EXPECT_EQ(second, "second line");
}

TEST(TUBULFileUtils, testOpenInputFileEmpty)
{
    const std::string filename = "tubul_test_open_input_empty.txt";
    {
        std::ofstream out(filename);
    }
    bool readSomething = true;
    {
        const auto in = TU::openInputFile(filename);
        std::string line;
        readSomething = static_cast<bool>(std::getline(*in, line));
    }
    std::remove(filename.c_str());
    EXPECT_FALSE(readSomething);
}

TEST(TUBULFileUtils, testOpenInputFileMissing)
{
    //The error names the file and the reason (the system's message, such as
    //"No such file or directory").
    std::string message;
    try { TU::openInputFile("tubul_test_open_input_no_such_file.txt"); }
    catch (const std::exception& error) { message = error.what(); }
    EXPECT_NE(message.find("'tubul_test_open_input_no_such_file.txt': "), std::string::npos) << message;
}
