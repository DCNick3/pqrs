//
// Created by dcnick3 on 2/10/21.
//

#include <pqrs/qr_easy_pipeline.h>

#include <pqrs/binarization.h>
#include <pqrs/grayscale.h>
#include <pqrs/finder_pattern_detector.h>
#include <pqrs/linear_external_contours.h>
#include <pqrs/contour_edge_intensity.h>
#include <pqrs/qr_scanner.h>

namespace pqrs {

    std::vector<scanned_qr> easy_scan_qr_codes(pqrs::gray_u8 const& gray) {
        std::vector<pqrs::finder_pattern> finder_patterns;
        {
            auto bin = pqrs::binarize(gray, 0.005);

            auto contours = pqrs::linear_external_contours_in_place(bin);

            for (auto const& contour : contours) {
                auto intensivity = pqrs::contour_edge_intensity(gray, contour);
                auto diff = std::abs(intensivity.first - intensivity.second);
                if (diff > 3) {
                    auto tetragon = pqrs::contour_to_tetragon(contour);
                    std::optional<pqrs::finder_pattern> position_pattern;
                    if (tetragon)
                        position_pattern = pqrs::check_finder_pattern(*tetragon, gray);

                    if (position_pattern)
                        finder_patterns.emplace_back(*position_pattern);
                }
            }
        }

        return pqrs::scan_qr_codes(gray, finder_patterns);
    }

    // take ownership so that we will be able to delete the original image after converting to grayscale
    std::vector<scanned_qr> easy_scan_qr_codes(color_u8 image) {
        auto gray = pqrs::grayscale(image);

        image.resize({0, 0, 0});

        return easy_scan_qr_codes(gray);
    }
}
