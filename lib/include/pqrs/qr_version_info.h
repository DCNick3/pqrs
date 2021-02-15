//
// Created by dcnick3 on 2/7/21.
//

#pragma once

#include <pqrs/point2d.h>
#include <pqrs/qr_ecc_decoder.h>

#include <optional>

#include <xtensor/xtensor.hpp>

namespace pqrs {

    struct ec_blocks_info {
        struct ecb {
            int _count;
            int _data_codewords;
        };

        int _ec_code_words_per_block;

        ecb _first_block_group;
        std::optional<ecb> _second_block_group;

        constexpr inline ec_blocks_info()
                : _ec_code_words_per_block(0), _first_block_group(), _second_block_group()
        {}

        constexpr inline ec_blocks_info(int ec_code_words_per_block, ecb first_block_group)
                : _ec_code_words_per_block(ec_code_words_per_block), _first_block_group(first_block_group)
        {}

        constexpr inline ec_blocks_info(int ec_code_words_per_block, ecb first_block_group, ecb second_block_group)
                : _ec_code_words_per_block(ec_code_words_per_block), _first_block_group(first_block_group),
                    _second_block_group(second_block_group)
        {}

        [[nodiscard]] inline int get_total_data_codewords() const {
            return _first_block_group._data_codewords * _first_block_group._count +
                    (_second_block_group ? _second_block_group->_data_codewords * _second_block_group->_count : 0);
        }
    };

    struct version_info {
        static constexpr int max_alignment = 7;

        int _version_number;

        int _alignment_count;
        std::array<int, max_alignment> _alignment;

        int _total_code_words;

        std::array<ec_blocks_info, 4> _block_info;

        constexpr inline version_info()
            : _version_number(0), _alignment_count(0), _alignment(), _total_code_words(0), _block_info()
        {}

        [[nodiscard]] inline ec_blocks_info const& ec_blocks_for_level(error_level ecc) const {
            return _block_info[static_cast<int>(ecc)];
        }

        constexpr inline version_info(int version_number, int alignment_count, std::array<int, max_alignment> alignment,
                                      int total_code_words, ec_blocks_info l, ec_blocks_info m, ec_blocks_info q, ec_blocks_info h)
                : _version_number(version_number), _alignment_count(alignment_count),
                _alignment(alignment), _total_code_words(total_code_words), _block_info() {
            _block_info[static_cast<int>(error_level::L)] = l;
            _block_info[static_cast<int>(error_level::M)] = m;
            _block_info[static_cast<int>(error_level::Q)] = q;
            _block_info[static_cast<int>(error_level::H)] = h;
        }

        [[nodiscard]] inline xt::xtensor<std::optional<point2d>, 2> alignment_grid() const {
            decltype(alignment_grid())::shape_type shape = {std::size_t(_alignment_count), std::size_t(_alignment_count)};
            decltype(alignment_grid()) res(shape);
            for (int row = 0; row < _alignment_count; row++)
                for (int col = 0; col < _alignment_count; col++) {
                    if ((row == 0 && col == 0)
                        || (row == 0 && col == _alignment_count - 1)
                        || (row == _alignment_count - 1 && col == 0)
                    )
                        res(row, col) = {};
                    else
                        res(row, col) = {{_alignment[col], _alignment[row]}};
                }
            return res;
        }

        [[nodiscard]] inline std::vector<point2d> alignment_locations() const {
            auto grid = alignment_grid();
            std::vector<point2d> res;
            for (int row = 0; row < _alignment_count; row++)
                for (int col = 0; col < _alignment_count; col++)
                    if (grid(row, col))
                        res.emplace_back(*grid(row, col));
            return res;
        }
    };

    version_info const& get_version_info(int version);
}