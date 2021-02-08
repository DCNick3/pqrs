//
// Created by dcnick3 on 1/29/21.
//

#include <pqrs/contour_edge_intensity.h>
#include <pqrs/interpolation.h>

#include <cmath>

namespace pqrs {
    static const int contour_samples = 30;
    static const int tangent_samples = 1;
    static const float tangent_step = 1.5f;

    std::pair<float, float> contour_edge_intensity(gray_u8 const& image, contour_container::inner const& contour) {
        float width = image.shape(1);
        float height = image.shape(0);

        // How many pixels along the contour it will step between samples
        int step;
        if (contour.size() <= contour_samples)
            step = 1;
        else
            step = (int) contour.size() / contour_samples;

        // Want the local tangent. How many contour points forward it will sample to get the tangent
        auto sample = std::max(1, std::min(step / 2, 5));

        auto edge_outside = 0.f, edge_inside = 0.f;
        auto total_inside = 0;
        auto total_outside = 0;

        // traverse the contour
        for (int i = 0; i < contour.size(); i += step) {
            auto a = contour[i];
            auto b = contour[(i + sample) % contour.size()];

            // compute the tangent using the two points
            auto dx = (float) (b.x() - a.x()), dy = (float) (b.y() - a.y());
            auto r = std::sqrt(dx * dx + dy * dy);
            dx /= r;
            dy /= r;

            // sample points tangent to the contour but not the contour itself
            for (int j = 0; j < tangent_samples; j++) {
                float x, y;
                float length = (float) (j + 1) * tangent_step;

                x = (float) a.x() + length * dy;
                y = (float) a.y() - length * dx;
                if (x >= 0 && y >= 0 && x <= width - 1 && y <= height - 1) {
                    edge_outside += interpolate_bilinear(image, x, y);
                    total_outside++;
                }

                x = (float) a.x() - length * dy;
                y = (float) a.y() + length * dx;
                if (x >= 0 && y >= 0 && x <= width - 1 && y <= height - 1) {
                    edge_inside += interpolate_bilinear(image, x, y);
                    total_inside++;
                }
            }
        }

        edge_outside /= total_outside;
        edge_inside /= total_inside;

        return std::make_pair(edge_outside, edge_inside);
    }
}
