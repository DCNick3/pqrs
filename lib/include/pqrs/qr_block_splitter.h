//
// Created by dcnick3 on 2/10/21.
//

#pragma once

#include <pqrs/qr.h>

#include <vector>
#include <cstdint>

namespace pqrs {
    std::vector<qr_block> split_blocks(std::vector<std::uint8_t> const& raw_data, int version, error_level error_level);
}

