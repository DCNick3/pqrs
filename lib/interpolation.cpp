//
// Created by dcnick3 on 1/29/21.
//

#include <pqrs/interpolation.h>

namespace pqrs {

	float interpolate_bilinear(gray_u8 const& image, float x, float y) {
		int h = image.shape(0), w = image.shape(1);

		// clip x and y
		x = x < 0 ? 0 : x > w-1 ? w-1 : x;
		y = y < 0 ? 0 : y > h-1 ? h-1 : y;

		// find source points
		int p1x = x, p1y = y;
		int p2x = std::min(p1x+1, w-1);
		int p2y = std::min(p1y+1, h-1);

		// find offset inside one pixel
		float dx = x - p1x;
		float dy = y - p1y;

		// find color of intermediate points
		float res1 = image(p1y, p1x) * (1 - dx) + image(p1y, p2x) * dx;
		float res2 = image(p2y, p1x) * (1 - dx) + image(p2y, p2x) * dx;

		// find color of result point
		return res1 * (1 - dy) + res2 * dy;
	}
}
