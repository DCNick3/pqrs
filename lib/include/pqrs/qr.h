//
// Created by dcnick3 on 2/10/21.
//

#pragma once

#include <vector>
#include <cstdint>

namespace pqrs {
    enum class error_level {
        L = 0b01,
        M = 0b00,
        Q = 0b11,
        H = 0b10,
    };
    enum class mask_type {
        M000 = 0b000,
        M001 = 0b001,
        M010 = 0b010,
        M011 = 0b011,
        M100 = 0b100,
        M101 = 0b101,
        M110 = 0b110,
        M111 = 0b111,
    };
    struct qr_format {
        error_level _error_level;
        mask_type _mask_type;
    };
    struct qr_block {
        std::size_t _data_size;
        std::vector<std::uint8_t> _data_and_ecc;
    };

}