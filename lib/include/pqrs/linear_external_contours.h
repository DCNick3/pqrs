//
// Created by dcnick3 on 1/25/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/contour_container.h>

namespace pqrs {
    contour_container linear_external_contours_in_place(gray_u8& bin_image);
    contour_container linear_external_contours(gray_u8 const& bin_image);
}