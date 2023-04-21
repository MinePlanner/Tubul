//
// Created by Carlos Acosta on 13-04-23.
//
#include "tubul.h"
#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>





TEST(TUBULGraph, testBasic) {

    TU::Graph::DAG dag{{
                               {{1, 1}, {2, 1}},
                               {{2, 1}},
                               {{1, 1}}
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


}
