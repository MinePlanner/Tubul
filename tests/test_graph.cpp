//
// Created by Carlos Acosta on 13-04-23.
//
#include "tubul.h"
#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>





TEST(TUBULGraph, testBasic) {

    TU::Graph::SparseWeightDirected dag{{}, {}, {
                               {{1, 0}, {2, 0}, {3,2}, {5,2}},
                               {{2, 1}, {4,1}, {5,0}},
                               {{1, 0},{4,0}},
                               {{4,3}, {5,3}},
                               {{1,2}},
                               { }
                       }
    };

    std::string filename("test.graph");
    TU::Graph::IO::Text::write(dag,filename);
    auto textg = TU::Graph::IO::Text::read("test.graph");
    EXPECT_EQ(true, equal(dag, textg));

    std::string filename2("testbin.graph");
    TU::Graph::IO::Binary::write(dag,filename2);
    auto bing = TU::Graph::IO::Binary::read("testbin.graph");
    EXPECT_EQ(true, equal(dag, bing));

    std::string filename3("testencoded.graph");
    TU::Graph::IO::Encoded::write(dag,filename3);
    auto binenc = TU::Graph::IO::Encoded::read("testencoded.graph");
    EXPECT_EQ(true, equal(dag, binenc));

}
TEST(TUBULGraph, testPrecedencesFormat) {
//If you want do do some tests, get a precedences file and point to it here. I
//was not sure how to properly add a prec file that i would know how to reach
//from the test.
#if 0
    auto start = TU::now();
    std::string filename("Pueblo_Viejo.prec");
    auto dag = TU::Graph::IO::Prec::read(filename);
    std::cout << " Parsing " << filename << " tool " << TU::elapsed(start) << "s" << std::endl;

    std::string out("publocopy.prec");
    TU::Graph::IO::Prec::write(dag, out);
#endif
}
