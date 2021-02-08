//
// Created by dcnick3 on 1/29/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/contour_container.h>

namespace pqrs {
    std::pair<float, float> contour_edge_intensity(gray_u8 const& image, contour_container::inner const& contour);
}