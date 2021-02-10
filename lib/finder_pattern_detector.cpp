//
// Created by dcnick3 on 1/31/21.
//

#include <pqrs/finder_pattern_detector.h>
#include <pqrs/interpolation.h>

namespace pqrs {

    namespace {
        constexpr int thresh_number_of_samples = 35;
        constexpr float thresh_corner_offset = 1.f;
        constexpr float thresh_normal_distance = 1.f;
        constexpr float min_edge_intensity = 6;
        constexpr int sample_period = 5; // sample 5 points per module

        bool check_ratio_along_the_line(tetragon const& tetragon, gray_u8 const& image, float thresh, int c0, int c1) {
            auto s0 = tetragon_get_side(tetragon, c0);
            auto s1 = tetragon_get_side(tetragon, c1);
            auto a = (s0.first + s0.second) / 2;
            auto b = (s1.first + s1.second) / 2;

            std::array<float, sample_period * 9 + 1> samples{};

            // sample the line so that 7 modules are inside the square and 2 modules + 1 pixel are outside

            float inside_size = sample_period * 7;

            for (int i = 0; i < samples.size(); i++) {
                auto t = (float)(i - sample_period) / inside_size;
                auto p = a * (1.f - t) + b * t;
                samples[i] = interpolate::bilinear(image, p);
            }

            // encode the samples as RLE

            std::vector<std::pair<bool, int>> rle;

            rle.emplace_back(samples[0] > thresh, 1);

            for (int i = 1; i < samples.size(); i++) {
                auto value = samples[i] > thresh;
                if (value == rle.rbegin()->first) {
                    rle.rbegin()->second++;
                } else {
                    rle.emplace_back(value, 1);
                }
            }

            // find 1:1:3:1:1 pattern in RLE

            // if too simple or too complex reject
            if (rle.size() < 5 || rle.size() > 9)
                return false;
            // detect finder pattern inside RLE
            for (int i = 0; i + 5 <= rle.size(); i++) {
                if (rle[i].first)
                    continue;

                // 1:1:3:1:1
                // b:w:b:w:b

                int black0 = rle[i].second;     // 1
                int black1 = rle[i + 2].second; // 3
                int black2 = rle[i + 4].second; // 1

                int white0 = rle[i + 1].second; // 1
                int white1 = rle[i + 3].second; // 1

                // the center black area can get exagerated easily
                if (black0 < 0.4*white0 || black0 > 3*white0)
                    continue;
                if (black2 < 0.4*white1 || black2 > 3*white1)
                    continue;

                int black02 = black0 + black2;

                if (black1 >= black02 && black1 <= 2 * black02)
                    return true;
            }

            return false;
        }
    }

    std::optional<finder_pattern> check_finder_pattern(tetragon const& tetragon, gray_u8 const& gray) {

        // compute the threshold using the info inside and outside the edge
        // then check that this pattern has 1:1:3:1:1 ratio of black & white modules
        // (see, for example, https://www.keyence.com/ss/products/auto_id/barcode_lecture/basic_2d/qr/)

        double total_sum_outside = 0.0;
        double total_sum_inside = 0.0;

        for (int side = 0; side < 4; side++) {
            // go along the edge and sample values inside and outside of it
            auto [a, b] = tetragon_get_side(tetragon, side);
            //auto a = tetragon[side];
            //auto b = tetragon[(side + 1) % 4];

            auto side_length = (b - a).norm();

            auto t_0 = 0.f + thresh_corner_offset / side_length;
            auto t_1 = 1.f - thresh_corner_offset / side_length;

            auto normal_vector = vector2d((b - a).y(), -(b - a).x());
            normal_vector /= (float)normal_vector.norm();

            double sum_outside = 0.0;
            double sum_inside = 0.0;

            for (int sample = 0; sample < thresh_number_of_samples; sample++) {
                auto t = (float)(t_0 + (t_1 - t_0) * sample / thresh_number_of_samples);
                auto p = a * (1.f - t) + b * t;

                //std::cout << t << " " << p.x() << " " << p.y() << "\n";

                auto p_outside = p + normal_vector * thresh_normal_distance;
                auto p_inside = p - normal_vector * thresh_normal_distance;

                sum_outside += interpolate::bilinear(gray, p_outside);
                sum_inside += interpolate::bilinear(gray, p_inside);
            }

            total_sum_outside += sum_outside / thresh_number_of_samples;
            total_sum_inside += sum_inside / thresh_number_of_samples;
        }

        total_sum_inside /= 4;
        total_sum_outside /= 4;

        if (total_sum_inside > total_sum_outside) {
            // looks like a white blob. reject
            return {};
        }

        auto threshold = (total_sum_inside + total_sum_outside) / 2;

        // we check ratio along the horizontal and vertical lines making up a cross centered at our candidate tetragon
        if (!check_ratio_along_the_line(tetragon, gray, threshold, 0, 2) &&
            !check_ratio_along_the_line(tetragon, gray, threshold, 1, 3)) {
            return {};
        }

        finder_pattern res;
        res.poly = tetragon;
        res.center = (tetragon[0] + tetragon[1] + tetragon[2] + tetragon[3]) / 4;
        res.gray_threshold = threshold;

        return res;
    }
}
