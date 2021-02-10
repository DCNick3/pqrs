//
// Created by dcnick3 on 2/1/21.
//

#pragma once

#include <pqrs/dynamic_bitset.h>
#include <pqrs/qr.h>

#include <optional>

namespace pqrs {

    std::optional<qr_format> try_decode_qr_format(dynamic_bitset const& bits);
    std::optional<int> try_decode_qr_version(dynamic_bitset const& bits);
    bool correct_qr_errors(qr_block& block);

}
