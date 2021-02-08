//
// Created by dcnick3 on 2/8/21.
//

#pragma once

#include <pqrs/homography_dlt.h>
#include <pqrs/dynamic_bitset.h>
#include <pqrs/image.h>
#include <pqrs/qr_decoder.h>

namespace pqrs {
    dynamic_bitset read_bitset(int version, qr_format format, std::function<bool(int, int)> const& sampler);
}
