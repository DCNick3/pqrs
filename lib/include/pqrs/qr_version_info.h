//
// Created by dcnick3 on 2/7/21.
//

#pragma once

#include <pqrs/point2d.h>
#include <pqrs/qr_decoder.h>

namespace pqrs {
    struct block_info {
        int _code_words_per_block, _data_codewords, _ecc_blocks;

        constexpr inline block_info()
                : _code_words_per_block(0), _data_codewords(0), _ecc_blocks(0)
        {}

        constexpr inline block_info(int code_words_per_block, int data_codewords, int ecc_blocks)
                : _code_words_per_block(code_words_per_block),
                _data_codewords(data_codewords), _ecc_blocks(ecc_blocks)
        {}
    };

    struct version_info {
        static constexpr int max_alignment = 7;

        int _alignment_count;
        std::array<int, max_alignment> _alignment;

        int _total_code_words;

        std::array<block_info, 4> _block_info;

        constexpr inline version_info()
            : _alignment_count(0), _alignment(), _total_code_words(0)
        {}

        [[nodiscard]] inline block_info const& level(error_level ecc) const {
            return _block_info[static_cast<int>(ecc)];
        }

        constexpr inline version_info(int alignment_count, std::array<int, max_alignment> alignment,
                                      int total_code_words, block_info l, block_info m, block_info q, block_info h)
                : _alignment_count(alignment_count), _alignment(alignment), _total_code_words(total_code_words) {
            _block_info[static_cast<int>(error_level::L)] = l;
            _block_info[static_cast<int>(error_level::M)] = m;
            _block_info[static_cast<int>(error_level::Q)] = q;
            _block_info[static_cast<int>(error_level::H)] = h;
        }

        [[nodiscard]] std::vector<point2d> alignment_locations() const {
            std::vector<point2d> res;
            for (int row = 0; row < _alignment_count; row++)
                for (int col = 0; col < _alignment_count; col++) {
                    if ((row == 0 && col == 0)
                            || (row == 0 && col == _alignment_count - 1)
                            || (row == _alignment_count - 1 && col == 0))
                        continue;
                    res.emplace_back(_alignment[col], _alignment[row]);
                }
            return res;
        }
    };

    version_info const& get_version_info(int version);
}