//
// Created by dcnick3 on 1/25/21.
//

#pragma once

#include <pqrs/image.h>

#include <xtensor/xtensor.hpp>
#include <xtensor/xio.hpp>
#include <xtensor/xview.hpp>

namespace pqrs {
    gray_u8 binarize(gray_u8 const& image, double block_size);
    void binarize_in_place(gray_u8& image, double block_size);
}


