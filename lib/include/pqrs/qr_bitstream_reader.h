//
// Created by dcnick3 on 2/8/21.
//

#pragma once

#include <pqrs/homography_dlt.h>
#include <pqrs/image.h>
#include <pqrs/qr_decoder.h>

#include <cstdint>
#include <vector>

namespace pqrs {
    // Is there a better variant for abstracting for grid reader than passing std::function?
    std::vector<std::uint8_t> read_raw_data(int version, qr_format format, std::function<bool(point2d)> const& sampler);
}
