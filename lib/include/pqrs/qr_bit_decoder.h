//
// Created by dcnick3 on 2/10/21.
//

#pragma once

#include <xtl/xspan.hpp>

#include <optional>
#include <string>

namespace pqrs {
    std::optional<std::string> decode_bits(xtl::span<std::uint8_t const> data, int version);
}
