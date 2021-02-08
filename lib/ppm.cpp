//
// Created by dcnick3 on 1/25/21.
//

#include <pqrs/ppm.h>

#include <sstream>

namespace pqrs {
    color_u8 load_ppm(std::string const& ppm) {
        if (ppm.size() < 3)
            throw bad_ppm_exception();
        if (ppm.substr(0, 2) != "P6")
            throw bad_ppm_exception();

        std::istringstream ss(std::string(ppm.substr(3)));
        unsigned int width, height, maxval;
        ss >> width >> height >> maxval;

        if (!(width > 0 && height > 0 && maxval == 255))
            throw bad_ppm_exception();

        auto res = color_u8::from_shape({ height, width, 3 });

        while (ss.get() == 10) ; // skip newlines
        // TODO: support the format better: any whitespace should be skipped
        // Also, there are comments (GIMP actually emits them...)
        ss.unget();

        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                for (int k = 0; k < 3; k++) {
                    auto v = ss.get();
                    if (ss.eof())
                        throw bad_ppm_exception();

                    if (v < 0 || v > maxval)
                        throw bad_ppm_exception();
                    res(j, i, k) = v;
                }
            }
        }

        return res;
    }

    std::string save_ppm(xt::xarray<std::uint8_t> const& array) {
        std::ostringstream ss;
        unsigned width = array.shape(1), height = array.shape(0);
        ss << "P6\n" << width << " " << height << "\n255\n";

        assert(array.shape().size() == 2 || array.shape().size() == 3 && array.shape(2) == 3);
        auto color = array.shape().size() == 3;

        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                for (int k = 0; k < 3; k++) {
                    if (color)
                        ss.put(array(j, i, k));
                    else
                        ss.put(array(j, i));
                }
            }
        }

        return ss.str();
    }
}