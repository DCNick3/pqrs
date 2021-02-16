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

    struct detected_qr {
        int _version{};
        qr_format _format;
        std::vector<std::pair<vector2d, vector2d>> _homography_features;
        homography _homography;
        std::uint8_t _threshold;

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

        inline detected_qr(int version, const qr_format &format,
                           std::vector<std::pair<vector2d, vector2d>> homography_features,
                           homography homography,
                           std::uint8_t threshold)
                : _version(version), _format(format),
                  _homography_features(std::move(homography_features)),
                  _homography(std::move(homography)),
                  _threshold(threshold) {}
    };

    struct decoded_qr : detected_qr {
        std::string _decoded_content;

        inline decoded_qr(detected_qr detected_qr1, std::string decoded_content)
            : detected_qr(std::move(detected_qr1)), _decoded_content(std::move(decoded_content)) {}

        inline decoded_qr(int version, const qr_format &format,
                          std::vector<std::pair<vector2d, vector2d>> homography_features,
                          homography homography,
                          std::uint8_t threshold,
                          std::string decoded_content)
                : detected_qr(version, format, std::move(homography_features),
                              std::move(homography), threshold),
                  _decoded_content(std::move(decoded_content)) {}
    };

}