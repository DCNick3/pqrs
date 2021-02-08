//
// Created by dcnick3 on 1/25/21.
//

#include <pqrs/grayscale.h>

namespace pqrs {
    gray_u8 grayscale(color_u8 const& color) {
        assert(color.shape().size() == 3);
        assert(color.shape(2) == 3);
        auto res = gray_u8::from_shape({color.shape(0), color.shape(1)});

        for (int j = 0; j < color.shape(0); j++)
            for (int i = 0; i < color.shape(1); i++) {
                res(j, i) = ((int)color(j, i, 0) + color(j, i, 1) + color(j, i, 2)) / 3;
            }

        return res;
    }
}