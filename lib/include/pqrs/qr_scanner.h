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
    std::vector<detected_qr> detect_qr_codes(gray_u8 const& image,
                                             std::vector<finder_pattern> const& finder_patterns);

    std::optional<decoded_qr> decode_qr_code(gray_u8 const& image, detected_qr detected_qr);

    /*std::vector<decoded_qr> scan_qr_codes(gray_u8 const& image,
                                          std::vector<finder_pattern> const& finder_patterns);*/
}
