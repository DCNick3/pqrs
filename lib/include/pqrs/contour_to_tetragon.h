//
// Created by dcnick3 on 1/29/21.
//

#pragma once

#include <pqrs/contour_container.h>
#include <pqrs/vector2d.h>

#include <optional>
#include <array>

namespace pqrs {

    typedef std::array<vector2d, 4> tetragon;

    inline std::pair<vector2d, vector2d> tetragon_get_side(tetragon const& tetragon, int index) {
        auto a = tetragon[index];
        auto b = tetragon[(index + 1) % 4];
        return {a, b};
    }

    // outputs in clockwise orientation
    std::optional<tetragon> contour_to_tetragon(contour_container::inner const& contour);
}