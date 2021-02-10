//
// Created by dcnick3 on 1/25/21.
//

#include <iostream>
#include <fstream>

#include <pqrs/binarization.h>
#include <pqrs/ppm.h>
#include <pqrs/grayscale.h>
#include <pqrs/linear_external_contours.h>
#include <pqrs/interpolation.h>
#include <pqrs/contour_edge_intensity.h>
#include <pqrs/contour_to_tetragon.h>
#include <pqrs/finder_pattern_detector.h>
#include <pqrs/qr_decoder.h>
#include <pqrs/homography_dlt.h>
#include <pqrs/qr_scanner.h>
#include <pqrs/qr_bitstream_reader.h>

namespace {
    //std::vector<pqrs::check_finder_pattern()
}

int main(int argc, char* argv[]) {

    //freopen("in.ppm","r",stdin);
    //freopen("out.ppm","w",stdout);

    auto infile = "in.ppm";
    auto outfile = "out.ppm";

    if (argc >= 2) {
        infile = argv[1];
    }
    if (argc >= 3) {
        outfile = argv[2];
    }

    std::ifstream istream(infile);
    std::ofstream ostream(outfile);

    std::string ppm;
    while (!istream.eof()) {
        auto v = istream.get();
        if (!istream.eof())
            ppm.push_back(v);
    }

    auto img = pqrs::load_ppm(ppm);
    ppm = "";
    auto gray = pqrs::grayscale(img);
    auto bin = pqrs::binarize(gray, 0.005);

    auto contours = pqrs::linear_external_contours_in_place(bin);

    std::vector<pqrs::finder_pattern> finder_patterns;

    for (auto const& contour : contours) {
        auto intensivity = pqrs::contour_edge_intensity(gray, contour);
        auto diff = std::abs(intensivity.first - intensivity.second);
        if (diff > 3) {
            //std::cerr << intensivity.first << " / " << intensivity.second << ": ";

            auto tetragon = pqrs::contour_to_tetragon(contour);
            std::optional<pqrs::finder_pattern> position_pattern;
            if (tetragon) {
                position_pattern = pqrs::check_finder_pattern(*tetragon, gray);
            }

            //std::cerr << (tetragon ? "T" : "-")
            //          << (position_pattern ? "P" : "-") << " ";

            for (int i = 0; i < contour.size(); i++) {

                //std::cerr << "{ " << contour[i].x() << ", " << contour[i].y() << " }";

                img(contour[i].y(), contour[i].x(), 0) = 0;
                img(contour[i].y(), contour[i].x(), 1) = 0;
                img(contour[i].y(), contour[i].x(), 2) = 255;

                //if (i != contour.size() - 1) {
                //    std::cerr << ", ";
                //}
            }
            //std::cerr << std::endl;

            if (tetragon) {
                for (auto p : *tetragon) {
                    img(p.y(), p.x(), 0) = 0;
                    img(p.y(), p.x(), 1) = 255;
                    img(p.y(), p.x(), 2) = 0;
                }
            }

            if (position_pattern) {
                img(position_pattern->center.y(), position_pattern->center.x(), 0) = 255;
                img(position_pattern->center.y(), position_pattern->center.x(), 1) = 0;
                img(position_pattern->center.y(), position_pattern->center.x(), 2) = 0;

                finder_patterns.emplace_back(*position_pattern);
            }
        }
    }

    auto qr_codes = pqrs::scan_qr_codes(gray, finder_patterns);

    for (auto const& code : qr_codes) {
        int size = code.size();
        int oversampling = 1;
        int round = 2;
        pqrs::gray_u8 im({(size + round * 2U) * oversampling, (size + round * 2U) * oversampling});
        for (int i = -round * oversampling; i < (size + round) * oversampling; i++) {
            for (int j = -round * oversampling; j < (size + round) * oversampling; j++) {
                im(j + round * oversampling, i + round * oversampling) = (std::uint8_t)
                        pqrs::interpolate::bilinear(gray,
                                                   code._homography.map({
                                                       (float)i / (float)oversampling + .5f,
                                                       (float)j / (float)oversampling + .5f}));
            }
        }
        ostream << pqrs::save_ppm(im);
    }

    //ostream << pqrs::save_ppm(img);



    return 0;
}
