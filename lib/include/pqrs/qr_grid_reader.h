//
// Created by dcnick3 on 2/15/21.
//

#pragma once

#include <pqrs/image.h>
#include <pqrs/homography_dlt.h>
#include <pqrs/interpolation.h>
#include <pqrs/finder_pattern_detector.h>

#include <utility>

namespace pqrs {

    struct qr_grid {
        gray_u8 const& _image;
        homography _homography;

        qr_grid(const gray_u8 &image, homography homography) : _image(image), _homography(std::move(homography)) {}

        [[nodiscard]] virtual bool sample(point2d p) const = 0;

        [[nodiscard]] inline bool sample(int x, int y) const {
            return sample({x, y});
        }
    };

    struct qr_grid_local : qr_grid {
        std::uint8_t _threshold;
    public:
        inline qr_grid_local(gray_u8 const& image, uint8_t threshold, homography homography)
                : qr_grid(image, std::move(homography)), _threshold(threshold) {}

        [[nodiscard]] inline bool sample(point2d p) const override {
            auto mapped = _homography.map(vector2d(p) + vector2d(.5f, .5f));
            auto v = interpolate_bilinear(_image, mapped);
            return v < (float) _threshold;
        }

        using qr_grid::sample;

        static inline qr_grid_local from_finder(gray_u8 const& image, finder_pattern const& finder) {
            std::vector<std::pair<vector2d, vector2d>> pts;

            pts.push_back({{7, 0}, finder.poly[0]});
            pts.push_back({{7, 7}, finder.poly[1]});
            pts.push_back({{0, 7}, finder.poly[2]});
            pts.push_back({{0, 0}, finder.poly[3]});

            auto homo = estimate_homography(pts);

            return qr_grid_local(image, finder.gray_threshold, homo);
        }
    };

    struct qr_grid_global : qr_grid {
        int _size;
    public:
        inline qr_grid_global(gray_u8 const& image, homography homography, int size)
                : qr_grid(image, std::move(homography)), _size(size) {}

        [[nodiscard]] inline bool sample(point2d p) const override {
            auto mapped = _homography.map(vector2d(p) + vector2d(.5f, .5f));
            auto v = interpolate_bilinear(_image, mapped);

            std::vector<float> samples;
            samples.reserve(25);
            for (int i = -2; i <= 2; i++)
                for (int j = -2; j <= 2; j++) {
                    auto sp = p + point2d(i, j);
                    if (sp.x() >= 0 && sp.y() >= 0 && sp.x() < _size && sp.y() < _size)
                        samples.emplace_back(interpolate_bilinear(_image, _homography.map(vector2d(sp))));
                }

            float sum = .0f;
            for (auto& s : samples)
                sum += s;
            auto avg = sum / samples.size();

            return v < (float) avg;
        }

        using qr_grid::sample;
    };
}