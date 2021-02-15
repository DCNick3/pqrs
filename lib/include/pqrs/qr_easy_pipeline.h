//
// Created by dcnick3 on 2/10/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/qr.h>
#include <pqrs/finder_pattern_detector.h>

#include <vector>

namespace pqrs {
    struct easy_scan_result {
        std::vector<finder_pattern> finder_patterns;
        std::vector<decoded_qr> qrs;
    };

    easy_scan_result easy_scan_qr_codes(gray_u8 const& image);

    // take ownership so that we will be able to delete the original image after converting to grayscale
    easy_scan_result easy_scan_qr_codes(color_u8 image);
}