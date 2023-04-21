//
// Created by Carlos Acosta on 21-04-23.
//

#include "tubul_varint.h"

namespace TU
{


    size_t bytesAsVarint(uint64_t x) {
        if (x == 0)
            return 0;
        static constexpr auto size_of_x = sizeof(x);
        static constexpr auto byte_size = 8;
        auto zeroes = helper::clz(x);
        auto bits_used= ((byte_size*size_of_x) - zeroes);
        return (bits_used/7) + ((bits_used%7>0)?1:0);
    }

}