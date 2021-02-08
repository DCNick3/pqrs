//
// Created by dcnick3 on 1/25/21.
//

#pragma once

#include <pqrs/image.h>

#include <xtensor/xarray.hpp>

#include <string_view>

namespace pqrs {
    class bad_ppm_exception : std::runtime_error {
    public:
        bad_ppm_exception() : runtime_error("Bad ppm image") {}
    };

    color_u8 load_ppm(std::string const& ppm);

    // TODO: split this into two overloads for color_u8 and gray_u8
    std::string save_ppm(xt::xarray<std::uint8_t> const& array);
}