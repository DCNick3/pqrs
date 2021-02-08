//
// Created by dcnick3 on 2/4/21.
//

#pragma once

#include <cstdint>
#include <cassert>

#include <xtl/xdynamic_bitset.hpp>

namespace pqrs {
    typedef xtl::xdynamic_bitset<std::size_t> dynamic_bitset;

    inline std::uint32_t read_bitset(dynamic_bitset const& bitset, int index, int size) {
        assert(size <= 32);
        std::uint32_t res = 0;
        for (int i = index; i < index + size; i++) {
            bool r = bitset[i];
            res |= ((r ? 1 : 0) << (i - index));
        }
        return res;
    }
}
