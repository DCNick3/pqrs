//
// Created by dcnick3 on 2/8/21.
//

#include <pqrs/qr_bitstream_reader.h>
#include <pqrs/qr_version_info.h>
#include <pqrs/ppm.h>

#include <fstream>

namespace pqrs {

    namespace {
        class function_module_mask {
            dynamic_bitset _bits;
            int _matrix_size;
        public:
            explicit function_module_mask(int matrix_size) : _bits(matrix_size * matrix_size, false),
                                                             _matrix_size(matrix_size) {
            }

            void use(int x, int y) {
                assert(x >= 0 && x < _matrix_size && y >= 0 && y < _matrix_size);
                _bits[y * _matrix_size + x] = true;
            }

            [[nodiscard]] bool used(int x, int y) const {
                assert(x >= 0 && x < _matrix_size && y >= 0 && y < _matrix_size);
                return _bits[y * _matrix_size + x];
            }
        };

        function_module_mask make_used_mask(int version, version_info const& verinf) {
            auto matrix_size = version * 4 + 17;

            function_module_mask res(matrix_size);
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

            // add version (if encoded)
            if (version >= 7) {
                for (int i = 0; i < 6; i++)
                    for (int j = 0; j < 3; j++) {
                        res.use(i, j + matrix_size - 11);
                    }
                for (int i = 0; i < 3; i++)
                    for (int j = 0; j < 6; j++) {
                        res.use(i + matrix_size - 11, j);
                    }

            }

            return res;
        }
    }

    std::vector<std::uint8_t> read_raw_data(int version, qr_format format, std::function<bool(point2d)> const& sampler) {
        auto const& verinf = get_version_info(version);

        auto function_module_mask = make_used_mask(version, verinf);

        auto matrix_size = version * 4 + 17;

        gray_u8 fun_img({size_t(matrix_size), size_t(matrix_size)});
        for (int i = 0; i < matrix_size; i++)
            for (int j = 0; j < matrix_size; j++) {
                fun_img(j, i) = function_module_mask.used(i, j) ? 255 : 0;
            }

        int index = 0;

        auto direction = direction8::topright; // up
        bool right = true;
        point2d position = {matrix_size - 1, matrix_size - 1};
        auto mask = [&](point2d p) -> bool {
            auto j = p.x(), i = p.y();
            // i is row, j is column
            switch (format._mask_type) {
                case mask_type::M000: return (i + j) % 2 == 0;
                case mask_type::M001: return i % 2 == 0;
                case mask_type::M010: return j % 3 == 0;
                case mask_type::M011: return (i + j) % 3 == 0;
                case mask_type::M100: return (i / 2 + j / 3) % 2 == 0;
                case mask_type::M101: return (i * j) % 2 + (i * j) % 3 == 0;
                case mask_type::M110: return ((i * j) % 2 + (i * j) % 3) % 2 == 0;
                case mask_type::M111: return ((i + j) % 2 + (i * j) % 3) % 2 == 0;
                default:
                    std::terminate();
            }
        };
        auto read_bit = [&]() -> bool {
            while (true) {
                auto pos_old = position;
                if (right) {
                    right = false;
                    position += direction8::left;
                } else {
                    right = true;
                    position += direction;
                }

                if (position.y() < 0) {
                    // got to the top
                    // move to next column, change direction to down
                    position += point2d(-2, 1);
                    direction = direction8::bottomright;
                }
                if (position.y() >= matrix_size) {
                    // got to the bottom
                    // move to next column, change direction to up
                    position += point2d(-2, -1);
                    direction = direction8::topright;
                }

                if (position.x() == 6)
                    position.x()--;

                if (pos_old.x() < 0) {
                    throw std::runtime_error("Not enough data in the qr code. qr_version_info bug?");
                }

                if (!function_module_mask.used(pos_old.x(), pos_old.y())) {
                    //std::cerr << index++ << ": { " << pos_old.x() << ", " << pos_old.y() << " }\n";
                    // xor with mask
                    return sampler(pos_old) != mask(pos_old);
                }
            }
        };

        std::vector<std::uint8_t> res;
        for (int i = 0; i < verinf._total_code_words; i++) {
            std::uint8_t cw = 0;

            for (int j = 7; j >= 0; j--) {
                cw |= read_bit() ? (1U << j) : 0;
            }

            res.emplace_back(cw);
        }

        return res;
    }
}