//
// Created by dcnick3 on 1/26/21.
//

#include <pqrs/linear_external_contours.h>
#include <pqrs/contour_container.h>

// based on [1] and boofcv implementation

// [1] Fu Chang and Chun-jen Chen and Chi-jen Lu, "A linear-time component-labeling algorithm using contour
//        tracing technique" Computer Vision and Image Understanding, 2004

namespace pqrs {
    namespace {
        struct impl {
            gray_u8 &_in_image;
            size_t width;
            size_t height;
            size_t min_contour_length = 4;
            size_t max_contour_length;
            contour_container _contours;

            gray_u8::value_type starting_mark = 129;
            gray_u8::value_type seen = 128;
            gray_u8::value_type one = 255;
            gray_u8::value_type zero = 0;

            explicit impl(gray_u8 &in_image) : _in_image(in_image) {
                // the original algorithm pads the image to make the border black
                // In qr scanning application borders don't particularly matter, so instead of padding we just fill them black

                width = in_image.shape(1);
                height = in_image.shape(0);

                // determine the maximum possible size of a position pattern
                // contour size is maximum when viewed head one. Assume the smallest qrcode is 3x this width
                // 4 side in a square
                max_contour_length = std::min(width, height) * 4 / 3;

                for (ssize_t i = 0; i < width; i++)
                    _in_image(0, i) = zero;
                for (ssize_t j = 0; j < height; j++)
                    _in_image(j, width - 1) = zero;
                for (ssize_t i = width - 1; i >= 0; i--)
                    _in_image(height - 1, i) = zero;
                for (ssize_t j = height - 1; j >= 0; j--)
                    _in_image(j, 0) = zero;
            }

            [[nodiscard]] point2d find_not_zero(point2d p) const {
                while (p.x() < width) {
                    auto val = _in_image(p.y(), p.x());
                    if (val != zero)
                        return p;
                    p += direction8::right;
                }
                return p;
            }

            [[nodiscard]] point2d find_zero(point2d p) const {
                while (p.x() < width && _in_image(p.y(), p.x()) != zero) {
                    p += direction8::right;
                }
                return p;
            }

            bool tracer(point2d initial_p, bool external) {
                // TODO determine if it's ambigous or not. The number of times this test is
                // done could be reduced I think.
                // verify that it's external. If there are ones above it then it can't possibly be external
                if (external) {
                    if (_in_image(initial_p.y() - 1, initial_p.x() - 1) != zero ||
                        _in_image(initial_p.y() - 1, initial_p.x()) != zero ||
                        _in_image(initial_p.y() - 1, initial_p.x() + 1) != zero)
                        external = false;
                }

                auto local_max_contour_length = max_contour_length;

                if (!external) {
                    local_max_contour_length = std::numeric_limits<decltype(local_max_contour_length)>::max();
                }

                // start a contour here
                _contours.grow();
                auto dir = external ? direction8::topright : direction8::top;

                auto p = initial_p;

                auto search_not_zero = [&]() -> bool {
                    for (int k = 0; k < 8; k++) {
                        auto p1 = p + (dir + k);
                        if (_in_image(p1.y(), p1.x()) != zero) {
                            dir = dir + k;
                            return true;
                        }
                    }
                    return false;
                };
                // index of pixels in the image array
                // binary has a 1 pixel border which labeled lacks, hence the -1,-1 for labeled

                // give the first pixel a special marking
                _contours.add_point_to_tail(p);
                _in_image(p.y(), p.x()) = 129;

                // find the next one pixel.  handle case where its an isolated point
                if (!search_not_zero()) {
                    return true;
                }
                auto initialDir = dir;
                p += dir;

                // start the next search +2 away from the square it came from
                // the square it came from is the opposite from the previous 'dir'
                dir = (dir + 4) + 2;

                while (true) {
                    search_not_zero();

                    if (_in_image(p.y(), p.x()) != starting_mark) {
                        _in_image(p.y(), p.x()) = seen;
                    } else {
                        if (p != initial_p) {
                            // found an marker that was left when leaving a blob and it was ambiguous if it was external
                            // or internal region of zeros
                            return false;
                        } else if (dir == initialDir) {
                            return external;
                        }
                    }
                    if (_contours.size_of_tail() <= local_max_contour_length)
                        _contours.add_point_to_tail(p);

                    p += dir;
                    dir = (dir + 4) + 2;
                }
            }

            // the values used in this implementation do not fully map to the ones used in boofcv (signed/unsigned; binary scale)
            // 0 -> 0 (zero), 1 -> 255 (one), -1 -> 128 (seen), -2 -> 129 (starting_mark)
            void process() {
                point2d p{0, 0};
                for (; p.y() < height; p.y()++) {
                    p.x() = 0;
                    while (true) {
                        p = find_not_zero(p);

                        if (p.x() == width)
                            break;

                        // If this pixel has NOT already been labeled then trace until it runs into a labeled pixel or it
                        // completes the trace. If a labeled pixel is not encountered then it must be an external contour
                        auto val = _in_image(p.y(), p.x());
                        if (val == one) {
                            if (tracer(p, true)) {
                                int n = _contours.size_of_tail();
                                if (n < min_contour_length || n > max_contour_length)
                                    _contours.remove_tail();
                            } else {
                                // it was really an internal contour
                                _contours.remove_tail();
                            }
                        }

                        // It's now inside a ones blob. Move forward until it hits a 0 pixel
                        p = find_zero(p);

                        if (p.x() == width)
                            break;

                        // If this pixel has NOT already been labeled trace until it completes a loop or it encounters a
                        // labeled pixel. This is always an internal contour
                        if (_in_image(p.y(), p.x() - 1) == one) {
                            tracer(p + direction8::left, false);
                            _contours.remove_tail();
                        } else {
                            // Can't be sure if it's entering a hole or leaving the blob. This marker will let the
                            // tracer know it just traced an internal contour and not an external contour
                            _in_image(p.y(), p.x() - 1) = starting_mark;
                        }
                    }
                }
            }
        };
    }

    contour_container linear_external_contours_in_place(gray_u8& bin_image) {
        impl impl_class(bin_image);

        impl_class.process();

        return std::move(impl_class._contours);
    }

    contour_container linear_external_contours(gray_u8 const& bin_image) {
        gray_u8 copy = bin_image;
        return linear_external_contours(copy);
    }
}