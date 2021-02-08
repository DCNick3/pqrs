//
// Created by dcnick3 on 1/25/21.
//

#pragma once

#include <pqrs/image.h>

#include <xtensor/xtensor.hpp>

namespace pqrs {
    gray_u8 grayscale(color_u8 const& color);
}
