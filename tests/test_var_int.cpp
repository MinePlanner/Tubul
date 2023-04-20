//
// Created by Carlos Acosta on 20-04-23.
//
#include "tubul.h"
#include <gtest/gtest.h>


namespace helper{
    //I don't like too much the "specification" of the builtin functions,
    //but I don't want to make any assumptions and later find they were wrong.
    //Note that per spec, clz(0) is undefined.
    inline
    unsigned clz(unsigned int x) {
        return __builtin_clz(x);
    }

    inline
    unsigned clz(unsigned long x) {
        return __builtin_clzl(x);
    }

    inline
    unsigned clz(unsigned long long x) {
        return __builtin_clzll(x);
    }

    //Based on the function to get the number of leading zeroes
    //we can calculate how many bytes are used by a given number
    template <typename UnsignedType>
        requires std::unsigned_integral<UnsignedType>
    unsigned bytes_used(UnsignedType x ){
        if (x == 0)
            return 0;
        static constexpr auto size_of_x = sizeof(x);
        static constexpr auto byte_size = 8;
        auto zeroes = clz(x);
        auto bits_used= ((byte_size*size_of_x) - zeroes);
        return (bits_used/byte_size) + ((bits_used%byte_size>0)?1:0);
    }
}

struct var_int_buffer
{
    var_int_buffer():
            size_(0)
    {}

    void push_back(uint8_t v){
        assert( size_ < buf_.max_size() );

        buf_[size_] = v;
        ++size_;
    }

    uint8_t* data() {
        return std::addressof(buf_[0]);
    }

    uint8_t size(){
        return size_;
    }

    uint8_t& at( uint8_t pos){
        return buf_[pos];
    }
    const uint8_t& at( uint8_t pos) const{
        return buf_[pos];
    }

    struct iter
    {
        iter(const var_int_buffer& s, uint8_t pos):
                source(s),
                p(pos)
        {}

        const uint8_t&  operator*() { return source.at(p); }
        bool operator!=(const iter& other){ return p != other.p; }
        iter& operator++(){ ++p; return *this;}


        const var_int_buffer& source;
        uint8_t p;
    };

    iter begin() const { return iter(*this,0);}
    iter end() const { return iter(*this,size_);}

    static var_int_buffer zero() {
        var_int_buffer z;
        z.push_back(0);
        return z;
    }

private:
    std::array<uint8_t, 9> buf_;
    uint8_t size_;
};

size_t encoded_bytes(uint64_t x) {
    if (x == 0)
        return 0;
    static constexpr auto size_of_x = sizeof(x);
    static constexpr auto byte_size = 8;
    auto zeroes = helper::clz(x);
    auto bits_used= ((byte_size*size_of_x) - zeroes);
    return (bits_used/7) + ((bits_used%7>0)?1:0);
}

var_int_buffer to_varint_repr(uint64_t x) {
    if (x == 0 ){
        return var_int_buffer::zero();
    }
    //This function tells us how many bytes are required to encode the value x
    //And then drop 1 because we will always do at least the first loop over the number.
    auto i = encoded_bytes(x) - 1;

    //We grab 7 bits of the number, and turn on the bit 8 to
    //signal the continuation of the number and store that byte.
    var_int_buffer out;
    for (int j = 0; j <= i; j++) {
        out.push_back(((x >> ((i - j) * 7)) & 127) | 128);
    }
    //We turn off the continuation bit of the last byte.
    out.at(i) ^= 128;
    return out;
}

uint64_t from_varint_repr(const var_int_buffer &seq) {
    uint64_t r = 0;

    //Move current value 7 bits, and read the next byte but drop
    //the uppermost bit (the continuation)
    for (auto b : seq) {
        r = (r << 7) | (b & 127);
    }

    return r;
}


TEST(TUBULVariableIntRepr, testCountLeadingZeroes) {

    EXPECT_EQ(31, helper::clz(0x1U)); //1
    EXPECT_EQ(30, helper::clz(0x2U)); //2
    EXPECT_EQ(25, helper::clz(0x7FU));//127
    EXPECT_EQ(24, helper::clz(0x80U));//128
    EXPECT_EQ(24, helper::clz(0x8FU));//143
    EXPECT_EQ(63, helper::clz(0x1UL));
    EXPECT_EQ(62, helper::clz(0x2UL));
    EXPECT_EQ(57, helper::clz(0x7FUL));//127
    EXPECT_EQ(56, helper::clz(0x80UL));//128
    EXPECT_EQ(56, helper::clz(0x8FUL));//143
    //All my tests have showed that unsigned long long is just
    //the same as unsigned long. I added tests just in case we have to
    //work on an architecture where this actually matters so we notice
    //the change.
    EXPECT_EQ(63, helper::clz(0x1ULL));
    EXPECT_EQ(62, helper::clz(0x2ULL));
    EXPECT_EQ(57, helper::clz(0x7FULL));
    EXPECT_EQ(56, helper::clz(0x80ULL));
    EXPECT_EQ(56, helper::clz(0x8FULL));
}

TEST(TUBULVariableIntRepr, testCountBytes) {

    EXPECT_EQ(1, helper::bytes_used(0x1U)); //1
    EXPECT_EQ(1, helper::bytes_used(0xFU)); //15
    EXPECT_EQ(1, helper::bytes_used(0xFFU)); //255
    EXPECT_EQ(2, helper::bytes_used(0x100U));//256
    EXPECT_EQ(2, helper::bytes_used(0xFFFU)); //4095
    EXPECT_EQ(2, helper::bytes_used(0x1000U)); //4096
    EXPECT_EQ(3, helper::bytes_used(0xFFFFFU)); //65535
    EXPECT_EQ(3, helper::bytes_used(0x10000U)); //65536
    EXPECT_EQ(3, helper::bytes_used(0x100000U)); //1048576
    EXPECT_EQ(3, helper::bytes_used(0xFFFFFFU)); //16777215
    EXPECT_EQ(4, helper::bytes_used(0x1000000U)); //16777216

    uint32_t ival = 127; uint64_t lval = 127;
    EXPECT_EQ(helper::bytes_used(ival), helper::bytes_used(lval)); //1
    EXPECT_EQ(1, helper::bytes_used(lval)); //1
    ival = 255; lval = 255;
    EXPECT_EQ(helper::bytes_used(ival), helper::bytes_used(lval)); //1
    EXPECT_EQ(1, helper::bytes_used(lval)); //1
    ival = 256; lval = 256;
    EXPECT_EQ(helper::bytes_used(ival), helper::bytes_used(lval)); //1
    EXPECT_EQ(2, helper::bytes_used(lval)); //1
}

TEST(TUBULVariableIntRepr, testEnconding) {

    auto round_trip = [](uint64_t x, size_t expected_length){
        auto eb = encoded_bytes(x);
        if (x != 0)
            EXPECT_EQ(eb, expected_length);
        auto encoded = to_varint_repr(x);
        EXPECT_EQ(encoded.size(), expected_length);
        auto decoded = from_varint_repr(encoded);
        EXPECT_EQ(decoded, x);
    };
    round_trip(0,1);
    round_trip(1,1);
    round_trip(127,1);
    round_trip(128,2);
    round_trip(255,2);
    round_trip(16384,3);
    round_trip( 2097151,3);
    round_trip( 2097152,4);
    round_trip( 4194302,4);
    round_trip(3679899543542109203,9);
}