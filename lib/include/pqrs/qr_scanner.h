//
// Created by dcnick3 on 2/2/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/finder_pattern_detector.h>
#include <pqrs/homography_dlt.h>
#include <pqrs/qr_ecc_decoder.h>

#include <utility>

namespace pqrs {
    struct scanned_qr {
        int _version{};
        qr_format _format;
        homography _homography;

        [[nodiscard]] inline int size() const {
            return 17 + _version * 4;
        }

        inline scanned_qr(int version, const qr_format &format, homography homography) : _version(version),
                                                                                         _format(format),
                                                                                         _homography(std::move(homography)) {}
    };

    std::vector<scanned_qr> scan_qr_codes(gray_u8 const& image,
                                          std::vector<finder_pattern> const& finder_patterns);
}
