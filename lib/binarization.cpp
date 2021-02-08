//
// Created by dcnick3 on 1/25/21.
//

#include <pqrs/binarization.h>

#include <xtensor/xview.hpp>

namespace pqrs {
    namespace {
        struct impl {
            gray_u8 const &_in_image;
            gray_u8 &_out_image;
            gray_u8 _stats;
            int width;
            int height;
            int block_width{};
            int block_height{};


            impl(gray_u8 const &in_image, double block_size_rel, gray_u8 &out_image)
                    : _in_image(in_image), _out_image(out_image) {
                assert(in_image.shape().size() == 2);
                assert(out_image.shape() == in_image.shape());

                width = in_image.shape(1);
                height = in_image.shape(0);

                int block_size = std::floor(std::min(width, height) * block_size_rel);

                set_block_size(std::max(block_size, 2));

                _stats = gray_u8::from_shape({
                                                     static_cast<unsigned long>(height / block_height),
                                                     static_cast<unsigned long>(width / block_width)
                                             });
            }

            void set_block_size(int requested_block_size) {
                if (height < requested_block_size) {
                    block_height = height;
                } else {
                    int rows = height / requested_block_size;
                    block_height = height / rows;
                }

                if (width < requested_block_size) {
                    block_width = width;
                } else {
                    int cols = width / requested_block_size;
                    block_width = width / cols;
                }
            }

            std::uint8_t block_mean(int x0, int y0, int m_width, int m_height) {
                unsigned sum = 0;
                for (int j = y0; j < y0 + m_height; j++)
                    for (int i = x0; i < x0 + m_width; i++) {
                        sum += _in_image(j, i);
                    }
                return std::uint8_t((sum + 0.5) / m_width / m_height);
            }

            void compute_stats(int inner_width, int inner_height) {
                int bj, bi, j, i;

                for (j = 0, bj = 0; j < inner_height; j += block_height, bj++) {
                    for (i = 0, bi = 0; i < inner_width; i += block_width, bi++) {
                        _stats(bj, bi) = block_mean(i, j, block_width, block_height);
                    }

                    // handle the case where the image's width isn't evenly divisible by the block's width
                    if (inner_width != width) {
                        _stats(bj, bi) = block_mean(i, j, width - inner_width, block_height);
                    }
                }

                // handle the case where the image's height isn't evenly divisible by the block's height
                if (inner_height != height) {
                    for (i = 0, bi = 0; i < inner_width; i += block_width, bi++) {
                        _stats(bj, bi) = block_mean(i, j, block_width, height - inner_height);
                    }

                    if (inner_width != width) {
                        _stats(bj, bi) = block_mean(i, j, width - inner_width, height - inner_height);
                    }
                }
            }

            std::uint8_t get_block_threshhold(int bi0, int bj0) {
                int bi1, bj1;

                bi1 = std::min(_stats.shape(1) - 1, size_t(bi0) + 1);
                bj1 = std::min(_stats.shape(0) - 1, size_t(bj0) + 1);

                bi0 = std::max(0, bi0 - 1);
                bj0 = std::max(0, bj0 - 1);

                int mean = 0;

                for (int bj = bj0; bj <= bj1; bj++) {
                    for (int bi = bi0; bi <= bi1; bi++) {
                        mean += _stats(bj, bi);
                    }
                }
                mean /= (bj1 - bj0 + 1) * (bi1 - bi0 + 1);

                return mean;
            }

            void process_block(int bi, int bj) {
                size_t i0 = bi * block_width;
                size_t j0 = bj * block_height;

                size_t i1 = bi == _stats.shape(1) - 1 ? _in_image.shape(1) : (bi + 1) * block_width;
                size_t j1 = bj == _stats.shape(0) - 1 ? _in_image.shape(0) : (bj + 1) * block_height;

                auto thresh = get_block_threshhold(bi, bj);

                for (int j = j0; j < j1; j++)
                    for (int i = i0; i < i1; i++) {
                        std::uint8_t val = _in_image(j, i);
                        if (val > thresh)
                            _out_image(j, i) = 0;
                        else
                            _out_image(j, i) = 255;
                    }
            }

            void process() {
                // the edge blocks are extended to include the leftover pixels after integer division (width / block_width)
                int inner_width = width % block_width == 0 ?
                                  width : width - block_width - width % block_width;
                int inner_height = height % block_height == 0 ?
                                   height : height - block_height - height % block_height;

                compute_stats(inner_width, inner_height);

                for (int bj = 0; bj < _stats.shape(0); bj++)
                    for (int bi = 0; bi < _stats.shape(1); bi++) {
                        process_block(bi, bj);
                    }
            }
        };
    }

    gray_u8 binarize(gray_u8 const& image, double block_size) {
        auto res = gray_u8::from_shape(image.shape());
        impl impl_class(image, block_size, res);

        impl_class.process();

        return res;
    }

    void binarize_in_place(gray_u8& image, double block_size) {
        impl impl_class(image, block_size, image);

        impl_class.process();
    }
}