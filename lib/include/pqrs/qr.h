//
// Created by dcnick3 on 2/10/21.
//

#pragma once

#include <pqrs/homography_dlt.h>

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

    struct scanned_qr {
        int _version{};
        qr_format _format;
        homography _homography;

        std::string _decoded_content;

        [[nodiscard]] inline int size() const {
            return 17 + _version * 4;
        }

        [[nodiscard]] vector2d top_left() const {
            return _homography.map({0, 0});
        }

        [[nodiscard]] vector2d top_right() const {
            return _homography.map({(float)size(), 0});
        }

        [[nodiscard]] vector2d bottom_left() const {
            return _homography.map({0, (float)size()});
        }

        [[nodiscard]] vector2d bottom_right() const {
            return _homography.map({(float)size(), (float)size()});
        }

        inline scanned_qr(int version, const qr_format &format, homography homography,
                          std::string decoded_content)
                : _version(version), _format(format),
                  _homography(std::move(homography)), _decoded_content(std::move(decoded_content)) {}
    };

}