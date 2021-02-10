//
// Created by dcnick3 on 1/29/21.
//

#include <pqrs/interpolation.h>

// bilinear interpolation

namespace pqrs {
    namespace {
        inline float interp(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d,
                            float ax, float ay) {
            float val = (1.0f - ax) * (1.0f - ay) * (float)a; // (x,y)
            val += ax * (1.0f - ay) * (float)b; // (x+1,y)
            val += ax * ay * (float)c; // (x+1,y+1)
            val += (1.0f - ax) * ay * (float)d; // (x,y+1)

            return val;
        }

        template<typename Getter>
        inline float impl(Getter const& getter, float x, float y) {
            int xt = (int)x;
            int yt = (int)y;
            return interp(getter(yt, xt), getter(yt, xt + 1), getter(yt + 1, xt + 1), getter(yt + 1, xt),
                          x - (float) xt, y - (float) yt);
        }
    }

    float interpolate_bilinear(gray_u8 const& image, float x, float y) {
        if (x < 0 || y < 0 || x > image.shape(1) - 2.f || y > image.shape(0) - 2.f) {
            auto get = [&](int j, int i) -> float {
                i = std::max(0, i);
                j = std::max(0, j);
                i = std::min(i, (int)image.shape(1) - 1);
                j = std::min(j, (int)image.shape(0) - 1);
                return image(j, i);
            };

            return impl(get, x, y);
        }

        return impl(image, x, y);
    }
}