//
// Created by dcnick3 on 1/26/21.
//

#pragma once

#include <utility>
#include <cstdint>
#include <cmath>

#include <pqrs/direction8.h>

namespace pqrs {
    struct point2d : public std::pair<int, int> {
    public:
        constexpr inline point2d(int x, int y) : pair(x, y) {}
        constexpr inline explicit point2d(direction8 direction) {
            if (direction == direction8::right) {
                x() = 1; y() = 0;
            } else if (direction == direction8::bottomright) {
                x() = 1; y() = 1;
            } else if (direction == direction8::bottom) {
                x() = 0; y() = 1;
            } else if (direction == direction8::bottomleft) {
                x() = -1; y() = 1;
            } else if (direction == direction8::left) {
                x() = -1; y() = 0;
            } else if (direction == direction8::topleft) {
                x() = -1; y() = -1;
            } else if (direction == direction8::top) {
                x() = 0; y() = -1;
            } else if (direction == direction8::topright) {
                x() = 1; y() = -1;
            }
        }

        [[nodiscard]] constexpr inline int x() const { return first; }
        [[nodiscard]] constexpr inline int y() const { return second; }

        constexpr inline int& x() { return first; }
        constexpr inline int& y() { return second; }

        constexpr inline point2d operator+(point2d const& o) const { return {x() + o.x(), y() + o.y()}; }
        constexpr inline point2d operator-(point2d const& o) const { return {x() - o.x(), y() - o.y()}; }

        constexpr inline point2d operator+(direction8 o) const { return *this + point2d(o); }
        constexpr inline point2d operator-(direction8 o) const { return *this - point2d(o); }

        inline point2d& operator+=(direction8 o) { return *this = *this + o; }
        inline point2d& operator+=(point2d o) { return *this = *this + o; }

        [[nodiscard]] constexpr inline int norm_squared() const { return x() * x() + y() * y(); }
        [[nodiscard]] constexpr inline double norm() const { return std::sqrt((double)norm_squared()); }
        [[nodiscard]] constexpr inline int manhattan_norm() const { return std::abs(x()) + std::abs(y()); }
    };
}