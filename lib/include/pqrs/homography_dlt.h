//
// Created by dcnick3 on 2/2/21.
//

#pragma once

#include <pqrs/vector2d.h>
#include <pqrs/homo_vector2d.h>

#include <utility>
#include <xtensor/xfixed.hpp>

#include <vector>

namespace pqrs {
    typedef xt::xtensor_fixed<float, xt::xshape<3, 3>> homography_matrix;

    class homography {
        homography_matrix _matrix;
    public:
        explicit homography(homography_matrix matrix) : _matrix(std::move(matrix)) {}

        [[nodiscard]] inline vector2d map(vector2d point) const {
            auto z = _matrix(2, 0) * point.x() + _matrix(2, 1) * point.y() + _matrix(2, 2);
            auto x = (_matrix(0, 0) * point.x() + _matrix(0, 1) * point.y() + _matrix(0, 2)) / z;
            auto y = (_matrix(1, 0) * point.x() + _matrix(1, 1) * point.y() + _matrix(1, 2)) / z;
            return {x, y};
        }

        [[nodiscard]] inline homography_matrix matrix() const { return _matrix; }

        [[nodiscard]] homography inverse() const;
    };

    // calculate matrix H from pairs (x_i, x_i') such that x_i' = H x_i
    homography estimate_homography(std::vector<std::pair<vector2d, vector2d>> const& points);

    // calculate matrix H from pairs (x_i, x_i') such that x_i' = H x_i in homogeneous coordinates
    homography estimate_homography(std::vector<std::pair<homo_vector2d, homo_vector2d>> const& points);
}
