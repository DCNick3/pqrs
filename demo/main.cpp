//
// Created by dcnick3 on 1/25/21.
//

#include <iostream>
#include <fstream>

#include <pqrs/ppm.h>
#include <pqrs/qr_easy_pipeline.h>


int main(int argc, char* argv[]) {

    auto infile = "in.ppm";
    auto outfile = "out.ppm";

    if (argc >= 2) {
        infile = argv[1];
    }
    if (argc >= 3) {
        outfile = argv[2];
    }

    std::ifstream istream(infile);

    std::string ppm;
    while (!istream.eof()) {
        auto v = istream.get();
        if (!istream.eof())
            ppm.push_back(v);
    }

    auto img = pqrs::load_ppm(ppm);

    auto result = pqrs::easy_scan_qr_codes(std::move(img));

    for (auto const& qr : result.qrs) {
        auto const& c = qr._decoded_content;
        std::string https = "https://";
        if (c.size() >= https.size() && c.substr(0, https.size()) == https) {
            std::cout << c << std::endl;
            return 0;
        }
    }

    return -1;
}