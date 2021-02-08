//
// Created by dcnick3 on 1/26/21.
//

#pragma once

#include <xtensor/xtensor.hpp>

namespace pqrs {
    typedef xt::xtensor<std::uint8_t, 2> gray_u8;
    typedef xt::xtensor<std::uint8_t, 3> color_u8;
}
