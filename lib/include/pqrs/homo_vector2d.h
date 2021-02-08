//
// Created by dcnick3 on 2/4/21.
//

#pragma once

#include <pqrs/vector2d.h>

#include <tuple>

namespace pqrs {
    struct homo_vector2d : public std::tuple<float, float, float> {
    public:
        inline homo_vector2d(float x, float y, float w) : tuple(x, y, w) {}
        inline homo_vector2d() : homo_vector2d(0, 0, 0) {}
        explicit inline homo_vector2d(vector2d p) : homo_vector2d((float)p.x(), (float)p.y(), 1.f) {}

        [[nodiscard]] inline float x() const { return std::get<0>(*this); }
        [[nodiscard]] inline float y() const { return std::get<1>(*this); }
        [[nodiscard]] inline float w() const { return std::get<2>(*this); }

        inline float& x() { return std::get<0>(*this); }
        inline float& y() { return std::get<1>(*this); }
        inline float& w() { return std::get<2>(*this); }
    };
}
