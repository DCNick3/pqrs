//
// Created by dcnick3 on 1/30/21.
//

#pragma once

#include <pqrs/point2d.h>

namespace pqrs {
    struct vector2d : public std::pair<float, float> {
    public:
        inline vector2d(float x, float y) : pair(x, y) {}
        inline vector2d() : vector2d(0, 0) {}
        explicit inline vector2d(point2d p) : vector2d((float)p.x(), (float)p.y()) {}

        [[nodiscard]] inline float x() const { return first; }
        [[nodiscard]] inline float y() const { return second; }

        inline float& x() { return first; }
        inline float& y() { return second; }

        inline vector2d operator+(vector2d const& o) const { return {x() + o.x(), y() + o.y()}; }
        inline vector2d operator-(vector2d const& o) const { return {x() - o.x(), y() - o.y()}; }

        inline vector2d operator*(float o) const { return {x() * o, y() * o}; }
        inline vector2d operator/(float o) const { return {x() / o, y() / o}; }

        inline vector2d& operator+=(vector2d o) { *this = *this + o; return *this; }
        inline vector2d& operator-=(vector2d o) { *this = *this - o; return *this; }

        inline vector2d& operator*=(float o) { *this = *this * o; return *this; }
        inline vector2d& operator/=(float o) { *this = *this / o; return *this; }

        [[nodiscard]] inline double norm_squared() const { return x() * x() + y() * y(); }
        [[nodiscard]] inline double norm() const { return std::sqrt((double)norm_squared()); }
        [[nodiscard]] inline double manhattan_norm() const { return std::abs(x()) + std::abs(y()); }

        [[nodiscard]] inline vector2d normal() const { return *this / (float)norm(); }
    };
}

