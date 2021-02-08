//
// Created by dcnick3 on 1/31/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/vector2d.h>
#include <pqrs/contour_to_tetragon.h>
#include <pqrs/direction4.h>

namespace pqrs {

    struct finder_pattern {
        tetragon poly;
        vector2d center;
        float gray_threshold{};

        std::pair<vector2d, vector2d> operator[](int direction) const {
            return tetragon_get_side(poly, direction);
        }

        std::pair<vector2d, vector2d> operator[](direction4 direction) const {
            return operator[](direction.v());
        }
    };

    std::optional<finder_pattern> check_finder_pattern(tetragon const& tetragon, gray_u8 const& gray);
}
