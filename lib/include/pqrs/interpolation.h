//
// Created by dcnick3 on 1/29/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/vector2d.h>

namespace pqrs::interpolate {
    float bilinear(gray_u8 const& image, float x, float y);

    inline float bilinear(gray_u8 const& image, vector2d p) {
    	return bilinear(image, p.x(), p.y());
    }
}
