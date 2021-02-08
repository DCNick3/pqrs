//
// Created by dcnick3 on 1/30/21.
//

#pragma once

#include <pqrs/vector2d.h>
#include <pqrs/point2d.h>
#include <pqrs/contour_to_tetragon.h>

#include <optional>

namespace pqrs {
    inline int circular_mod(int a, int size) {
        if (a < 0) {
            a = size + a;
        }
        return a % size;
    }

    inline int circular_distance_positive(int a, int b, int size) {
        return circular_mod(b - a, size);
    }

    inline double line_segment_to_point_distance_squared(point2d v, point2d w,
                                                         point2d p) {
        // Return minimum distance between line segment vw and point p
        auto l2 = (v - w).norm_squared();  // i.e. |w-v|^2 -  avoid a sqrt
        if (l2 == 0.0) return (p - v).norm();   // v == w case
        // Consider the line extending the segment, parameterized as v + t (w - v).
        // We find projection of point p onto the line.
        // It falls where t = [(p-v) . (w-v)] / |w-v|^2
        // We clamp t from [0,1] to handle points outside the segment vw.
        auto a = p - v;
        auto b = w - v;

        auto t = std::max(0., std::min(1., (0. + a.x() * b.x() + a.y() * b.y()) / l2));
        auto x = v.x() + t * (w - v).x();// Projection falls on the segment
        auto y = v.y() + t * (w - v).y();
        auto dx = p.x() - x;
        auto dy = p.y() - y;
        return dx * dx + dy * dy;
    }

    inline double line_to_point_distance_squared(point2d v, point2d w, point2d p) {
        double a = v.y() - w.y();
        double b = w.x() - v.x();
        double c = v.x() * w.y() - w.x() * v.y();
        return std::abs(a * p.x() + b * p.y() + c) / std::sqrt(a * a + b * b);
    }

    inline float matrix_determinant(float a, float b, float c, float d) {
        return a*d - b*c;
    }

    inline std::optional<vector2d> lines_intersection(vector2d p11, vector2d p12, vector2d p21, vector2d p22) {
        //http://mathworld.wolfram.com/Line-LineIntersection.html

        auto detL1 = matrix_determinant(p11.x(), p11.y(), p12.x(), p12.y());
        auto detL2 = matrix_determinant(p21.x(), p21.y(), p22.x(), p22.y());
        auto x1mx2 = p11.x() - p12.x();
        auto x3mx4 = p21.x() - p22.x();
        auto y1my2 = p11.y() - p12.y();
        auto y3my4 = p21.y() - p22.y();

        vector2d res{0,0};

        auto xnom = matrix_determinant(detL1, x1mx2, detL2, x3mx4);
        auto ynom = matrix_determinant(detL1, y1my2, detL2, y3my4);
        auto denom = matrix_determinant(x1mx2, y1my2, x3mx4, y3my4);
        if(denom == 0.0)//Lines don't seem to cross
        {
            res.x() = NAN;
            res.y() = NAN;
            return {};
        }

        res.x() = xnom / denom;
        res.y() = ynom / denom;
        if(!std::isfinite(res.x()) || !std::isfinite(res.y())) //Probably a numerical issue
            return {};

        return res; //All OK
    }

    inline std::optional<vector2d> line_segments_intersection(vector2d p0, vector2d p1, vector2d p2, vector2d p3) {
        float s1_x, s1_y, s2_x, s2_y;
        s1_x = p1.x() - p0.x();     s1_y = p1.y() - p0.y();
        s2_x = p3.x() - p2.x();     s2_y = p3.y() - p2.y();

        float s, t;
        s = (-s1_y * (p0.x() - p2.x()) + s1_x * (p0.y() - p2.y())) / (-s2_x * s1_y + s1_x * s2_y);
        t = ( s2_x * (p0.y() - p2.y()) - s2_y * (p0.x() - p2.x())) / (-s2_x * s1_y + s1_x * s2_y);

        if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
        {
            // Collision detected
            auto x = p0.x() + (t * s1_x);
            auto y = p0.y() + (t * s1_y);
            return vector2d(x, y);
        }

        return {}; // No collision
    }

    inline std::optional<std::pair<int, vector2d>> tetragon_segment_intersection_any(tetragon const& t, vector2d p0, vector2d p1) {
        for (int i = 0; i < 4; i++) {
            auto [p2, p3] = tetragon_get_side(t, i);
            auto intersection = line_segments_intersection(p0, p1, p2, p3);
            if (intersection)
                return {{i, *intersection}};
        }
        return {};
    }

    inline float line_acute_angle(vector2d p0, vector2d p1, vector2d p2, vector2d p3) {
        auto v1 = (p1 - p0);
        auto v2 = (p3 - p2);

        auto value = v1.x() * v2.x() + v1.y() * v2.y();
        value /= v1.norm() * v2.norm();
        if (value > 1.f)
            value = 1.f;
        else if (value < -1.f)
            value = -1.f;

        auto res = std::acos(value);

        if (res > M_PI_2)
            res = M_PI - res;

        return res;
    }

    /**
     * Returns true if the cross product would result in a strictly positive z (e.g. z &gt; 0 ). If true then
     * the order is clockwise.
     *
     * v0 = a-b
     * v1 = c-b
     *
     * @param a first point in sequence
     * @param b second point in sequence
     * @param c third point in sequence
     * @return true if positive z
     */
    inline bool polygon_is_positive_z(vector2d a, vector2d b, vector2d c) {
        auto dx0 = a.x() - b.x();
        auto dy0 = a.y() - b.y();

        auto dx1 = c.x() - b.x();
        auto dy1 = c.y() - b.y();

        auto z = dx0 * dy1 - dy0 * dx1;

        return z > 0;
    }

    inline bool polygon_is_positive_z(point2d a, point2d b, point2d c) {
        return polygon_is_positive_z(vector2d(a.x(), a.y()),
                                     vector2d(b.x(), b.y()),
                                     vector2d(c.x(), c.y()));
    }

    template<typename T>
    inline std::uint8_t count_ones(T v) {
        std::uint8_t c; // c accumulates the total bits set in v
        for (c = 0; v; c++)
            v &= v - 1; // clear the least significant bit set
        return c;
    }
}
