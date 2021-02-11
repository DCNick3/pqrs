//
// Created by dcnick3 on 2/10/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/qr.h>

#include <vector>

namespace pqrs {
    std::vector<scanned_qr> easy_scan_qr_codes(gray_u8 const& image);

    // take ownership so that we will be able to delete the original image after converting to grayscale
    std::vector<scanned_qr> easy_scan_qr_codes(color_u8 image);
}