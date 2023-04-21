//
// Created by Carlos Acosta on 21-04-23.
//

#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <cassert>
#ifdef TUBUL_WINDOWS
#include <intrin.h>
#endif
namespace TU
{

    namespace helper{
#ifndef TUBUL_WINDOWS
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

#endif

#ifdef TUBUL_WINDOWS
        inline
        unsigned clz( uint32_t value ) {
            unsigned long leading_zero = 0;
            if ( _BitScanReverse( &leading_zero, value ) ) {
                return 31 - leading_zero;
            }
            else {
                // Same remarks as above
                return 32;
            }
        }

        inline
        unsigned clz( uint64_t value ) {
            unsigned long leading_zero = 0;
            if ( _BitScanReverse64( &leading_zero, value ) ) {
                return 63 - leading_zero;
            }
            else {
                // Same remarks as above
                return 64;
            }
        }
#endif
        //Based on the function to get the number of leading zeroes
        //we can calculate how many bytes are used by a given number
        template <typename UnsignedType>
            requires std::unsigned_integral<UnsignedType>
            inline
        unsigned bytesUsed(UnsignedType x ){
            if (x == 0)
                return 0;
            static constexpr auto size_of_x = sizeof(x);
            static constexpr auto byte_size = 8;
            auto zeroes = clz(x);
            auto bits_used= ((byte_size*size_of_x) - zeroes);
            return (bits_used/byte_size) + ((bits_used%byte_size>0)?1:0);
        }
    }

    struct VarIntBuffer
    {
        VarIntBuffer():
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

        uint8_t size() const{
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
            iter(const VarIntBuffer& s, uint8_t pos):
                    source(s),
                    p(pos)
            {}

            const uint8_t&  operator*() { return source.at(p); }
            bool operator!=(const iter& other) const { return p != other.p; }
            iter& operator++(){ ++p; return *this;}


            const VarIntBuffer& source;
            uint8_t p;
        };

        iter begin() const { return {*this,0};}
        iter end() const { return {*this,size_};}

        static VarIntBuffer zero() {
            VarIntBuffer z;
            z.push_back(0);
            return z;
        }

    private:
        std::array<uint8_t, 9> buf_;
        uint8_t size_;
    };

    size_t bytesAsVarint(uint64_t x);

    inline
    VarIntBuffer toVarint(uint64_t x) {
        if (x == 0 ){
            return VarIntBuffer::zero();
        }
        //This function tells us how many bytes are required to encode the value x
        //And then drop 1 because we will always do at least the first loop over the number.
        auto i = bytesAsVarint(x) - 1;

        //We grab 7 bits of the number, and turn on the bit 8 to
        //signal the continuation of the number and store that byte.
        VarIntBuffer out;
        for (int j = 0; j <= i; j++) {
            out.push_back(((x >> ((i - j) * 7)) & 127) | 128);
        }
        //Turn off the continuation bit of the last byte.
        out.at(i) ^= 128;
        return out;
    }

    inline
    uint64_t fromVarint(const VarIntBuffer &seq) {
        uint64_t r = 0;

        //Move current value 7 bits, and read the next byte but drop
        //the uppermost bit (the continuation)
        for (auto b : seq) {
            r = (r << 7) | (b & 127);
        }

        return r;
    }

}