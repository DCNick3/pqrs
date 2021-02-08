//
// Created by dcnick3 on 2/8/21.
//

#include <pqrs/qr_bitstream_reader.h>
#include <pqrs/qr_version_info.h>
#include <pqrs/ppm.h>

#include <fstream>

namespace pqrs {

    namespace {
        class used_mask {
            dynamic_bitset _bits;
            int _matrix_size;
        public:
            explicit used_mask(int matrix_size) : _bits(matrix_size * matrix_size, false),
                _matrix_size(matrix_size) {
            }

            void use(int x, int y) {
                _bits[y * _matrix_size + x] = true;
            }

            [[nodiscard]] bool used(int x, int y) const {
                return _bits[y * _matrix_size + x];
            }
        };

        used_mask make_used_mask(int matrix_size, version_info const& verinf) {
            used_mask res(matrix_size);
            // add right and bottom finder patterns & format region 1
            for (int i = 0; i < 8; i++)
                for (int j = 0; j < 9; j++) {
                    res.use(matrix_size - 8 + i, j);
                    res.use(j, matrix_size - 8 + i);
                }
            // add origin finder pattern & format region 0
            for (int i = 0; i < 9; i++)
                for (int j = 0; j < 9; j++) {
                    res.use(i, j);
                }
            // add timing patterns
            for (int i = 0; i < matrix_size; i++)
            {
                res.use(i, 6);
                res.use(6, i);
            }
            // add alignment patterns
            for (auto [x, y] : verinf.alignment_locations()) {
                for (int i = -2; i <= 2; i++)
                    for (int j = -2; j <= 2; j++) {
                        res.use(x + i, y + j);
                    }
            }
            return res;
        }
    }

    dynamic_bitset read_bitset(int version, qr_format format, std::function<bool(int, int)> const& sampler) {
        auto const& verinf = get_version_info(version);

        auto matrix_size = version * 4 + 17;

        auto used_matrix = make_used_mask(matrix_size, verinf);

        dynamic_bitset res;



        return {};
    }
}