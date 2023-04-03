//
// Created by Carlos Acosta on 03-04-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
#include <iostream>
#include <unordered_map>
#include <tuple>

TEST(TUBULHash, BasicTupleHash2) {

    using P2i = std::tuple<int,int>;
    TU::hash<P2i> hasher;
    P2i v1{10,20};
    auto val = hasher(v1);
    P2i v2{10,20};
    auto val2 = hasher(v2);
    EXPECT_EQ(val, val2);
    P2i v3{20,20};
    auto val3 = hasher(v3);
    EXPECT_NE(val, val3);
    P2i v4{20,10};
    auto val4 = hasher(v4);
    EXPECT_NE(val, val4);
}

TEST(TUBULHash, BasicTupleHash3) {
    using P3i = std::tuple<int,int, int>;
    TU::hash<P3i> hasher;

    P3i v1{10,20,500};
    auto val = hasher(v1);
    P3i v2{10,20, 100};
    val = hasher(v2);
    P3i v3{20,20,20};
    val = hasher(v3);
    P3i v4{20,10, 10};
    val = hasher(v4);
}

TEST(TUBULHash, BasicTupleHash3_float) {
    using P3f = std::tuple<float,float, float>;
    TU::hash<P3f> hasher;

    P3f v1{10,20,500};
    auto val = hasher(v1);
    P3f v2{10,20, 100};
    val = hasher(v2);
    P3f v3{20,20,20};
    val = hasher(v3);
    P3f v4{20,10, 10};
    val = hasher(v4);
}

TEST(TUBULHash, TupleHash3_misc) {
    using P3h = std::tuple<float,std::string, size_t>;
    TU::hash<P3h> hasher;

    P3h v1{10.0,"Pepito",500};
    auto val = hasher(v1);
    P3h v2{3.1415,"Juanito", 100};
    val = hasher(v2);
    P3h v3{20,"Pedrito",20};
    val = hasher(v3);
    P3h v4{20,"Juanelo", 10};
    val = hasher(v4);
}

TEST(TUBULHash, TupleHash3_umaps) {
    using P3h = std::tuple<float, std::string, size_t>;
    TU::hash<P3h> hasher;
    std::unordered_map<P3h, int, TU::hash<P3h> > container;

    P3h v1{10.0, "Pepito", 500};
    auto val = hasher(v1);
    container[v1] = 10;

    P3h other{ 10.0, "Pepito", 500};
    auto val2 = hasher(other);
    EXPECT_EQ(val,val2);
}

