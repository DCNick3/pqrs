//
// Created by dcnick3 on 2/10/21.
//

#include <pqrs/qr_block_splitter.h>
#include <pqrs/qr_version_info.h>

namespace pqrs {

    namespace {
        void add_blocks(std::vector<qr_block>& res, ec_blocks_info const& ec_group, ec_blocks_info::ecb const& ecb) {
            for (int i = 0; i < ecb._count; i++) {
                auto& r = res.emplace_back();
                r._data_size = ecb._data_codewords;
                r._data_and_ecc.reserve(ecb._data_codewords + ec_group._ec_code_words_per_block);
            }
        }
    }

    std::vector<qr_block>
    split_blocks(const std::vector<std::uint8_t> &raw_data, int version, error_level error_level) {
        auto const& version_info = get_version_info(version);
        auto const& blocks_info = version_info.ec_blocks_for_level(error_level);

        std::vector<qr_block> res;
        add_blocks(res, blocks_info, blocks_info._first_block_group);
        if (blocks_info._second_block_group)
            add_blocks(res, blocks_info, *blocks_info._second_block_group);

        xtl::span<std::uint8_t const> data_span(raw_data.data(), raw_data.size());

        // first read all the data
        bool full = false;
        while (!full) {
            full = true;
            for (auto& block : res) {
                if (block._data_size > block._data_and_ecc.size()) {
                    assert(!data_span.empty());
                    block._data_and_ecc.emplace_back(data_span[0]);
                    data_span = data_span.subspan(1);
                    full = false;
                }
            }
        }

        // now read the ecc
        full = false;
        while (!full) {
            full = true;
            for (auto& block : res) {
                if (block._data_size + blocks_info._ec_code_words_per_block > block._data_and_ecc.size()) {
                    assert(!data_span.empty());
                    block._data_and_ecc.emplace_back(data_span[0]);
                    data_span = data_span.subspan(1);
                    full = false;
                }
            }
        }

        assert(data_span.empty());

        return res;
    }
}