//
// Created by dcnick3 on 2/10/21.
//

#include <pqrs/qr.h>
#include <pqrs/qr_bit_decoder.h>

#include <utility>
#include <vector>
#include <cstdint>
#include <cassert>
#include <string>
#include <array>

namespace pqrs {
    namespace {
        class bit_source {
            xtl::span<const std::uint8_t> _bytes;
            std::size_t _byte_offset{};
            std::size_t _bit_offset{};

        public:
            explicit bit_source(xtl::span<const std::uint8_t> bytes)
                    : _bytes(bytes) {
            }

            [[nodiscard]] inline int get_bit_offset() const { return _bit_offset; }
            [[nodiscard]] inline int get_byte_offset() const { return _byte_offset; }

            [[nodiscard]] inline std::size_t available() const {
                return 8 * (_bytes.size() - _byte_offset) - _bit_offset;
            }

            inline std::uint32_t read_bits(std::size_t num_bits) {
                assert(num_bits >= 1 && num_bits <= 32);

                if (num_bits > available())
                    std::terminate();

                int result = 0;

                // First, read remainder from current byte
                if (_bit_offset > 0) {
                    std::size_t bits_left = 8 - _bit_offset;
                    int to_read = std::min(num_bits, bits_left);
                    std::size_t bits_to_not_read = bits_left - to_read;
                    int mask = (0xFF >> (8 - to_read)) << bits_to_not_read;
                    result = (_bytes[_byte_offset] & mask) >> bits_to_not_read;
                    num_bits -= to_read;
                    _bit_offset += to_read;
                    if (_bit_offset == 8) {
                        _bit_offset = 0;
                        _byte_offset++;
                    }
                }

                // Next read whole bytes
                if (num_bits > 0) {
                    while (num_bits >= 8) {
                        result = (result << 8) | (_bytes[_byte_offset] & 0xFF);
                        _byte_offset++;
                        num_bits -= 8;
                    }

                    // Finally read a partial byte
                    if (num_bits > 0) {
                        std::size_t bits_to_not_read = 8 - num_bits;
                        int mask = (0xFF >> bits_to_not_read) << bits_to_not_read;
                        result = (result << num_bits) | ((_bytes[_byte_offset] & mask) >> bits_to_not_read);
                        _bit_offset += num_bits;
                    }
                }

                return result;
            }
        };

        enum class mode {
            terminator = 0x0,
            numeric = 0x1,
            alphanumeric = 0x2,
            structured_append = 0x3,
            byte = 0x4,
            fnc1_first_portion = 0x5,
            eci = 0x7,
            kanji = 0x8,
            fnc1_second_portion = 0x9,
            hanzi = 0xd,
        };

        inline std::size_t get_character_count_bits(mode mode, int version) {
            if (version <= 9) {
                switch (mode) {
                    case mode::numeric:         return 10;
                    case mode::alphanumeric:    return 9;
                    case mode::byte:            return 8;
                    case mode::kanji:           return 8;
                    case mode::hanzi:           return 8;
                    default:
                        std::terminate();
                }
            } else if (version <= 26) {
                switch (mode) {
                    case mode::numeric:         return 12;
                    case mode::alphanumeric:    return 11;
                    case mode::byte:            return 16;
                    case mode::kanji:           return 10;
                    case mode::hanzi:           return 10;
                    default:
                        std::terminate();
                }
            } else {
                switch (mode) {
                    case mode::numeric:         return 14;
                    case mode::alphanumeric:    return 13;
                    case mode::byte:            return 16;
                    case mode::kanji:           return 12;
                    case mode::hanzi:           return 12;
                    default:
                        std::terminate();
                }
            }
        }

        std::array<char, 45> alphanum_chars ={ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
                                               'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                                               'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                               'Y', 'Z', ' ', '$', '%', '*', '+', '-', '.', '/', ':'};

        inline bool append_alphanum(std::string& result, unsigned index) {
            if (index >= alphanum_chars.size())
                return false;
            result.push_back(alphanum_chars[index]);
            return true;
        }

        bool decode_numeric(bit_source& bits, std::string& result, int count) {
            while (count >= 3) {
                if (bits.available() < 10) return false;
                auto data = bits.read_bits(10);
                if (data >= 1000) return false;

                if (!append_alphanum(result, data / 100) ||
                    !append_alphanum(result, (data / 10) % 10) ||
                    !append_alphanum(result, data % 10))
                    return false;

                count -= 3;
            }
            if (count == 2) {
                if (bits.available() < 7) return false;
                auto data = bits.read_bits(7);
                if (data >= 100) return false;

                if (!append_alphanum(result, data / 10) ||
                    !append_alphanum(result, data % 10))
                    return false;
            } else if (count == 1) {
                if (bits.available() < 4) return false;
                auto data = bits.read_bits(7);
                if (data >= 10) return false;
                return append_alphanum(result, data);
            }
            return true;
        }

        bool decode_alphanumeric(bit_source& bits, std::string& result, int count) {
            while (count > 1) {
                if (bits.available() < 11) return false;
                auto next_two_char_bits = bits.read_bits(11);

                if (!append_alphanum(result, next_two_char_bits / 45) ||
                    !append_alphanum(result, next_two_char_bits % 45))
                    return false;

                count -= 2;
            }
            if (count == 1) {
                if (bits.available() < 6) return false;
                return append_alphanum(result, bits.read_bits(6));
            }

            return true;
        }

        bool decode_byte(bit_source& bits, std::string& result, int count) {
            // Don't crash trying to read more bits than we have available.
            if (8 * count > bits.available())
                return false;

            std::string raw;
            raw.reserve(count);
            for (int i = 0; i < count; i++) {
                raw.push_back(bits.read_bits(8));
            }
            // TODO: port the encoding guessing code
            // For now - assume latin-1 (ISO-8859-1) (because west is superior) ((jk))

            // convert latin-1 to urf8
            for (std::uint8_t ch : raw) {
                if (ch < 0x80) {
                    result.push_back(ch);
                } else {
                    result.push_back(0xc0 | ch >> 6);
                    result.push_back(0x80 | (ch & 0x3f));
                }
            }
            return true;
        }
    }


    std::optional<std::string> decode_bits(xtl::span<const std::uint8_t> data, int version) {
        std::string result;
        result.reserve(50);

        bit_source bits(data);

        mode current_mode;
        do {
            if (bits.available() < 4) {
                // OK, assume we're done. Really, a TERMINATOR mode should have been recorded here
                current_mode = mode::terminator;
            } else {
                current_mode = static_cast<mode>(bits.read_bits(4)); // mode is encoded by 4 bits
            }
            switch (current_mode) {
                case mode::terminator:
                    break;
                case mode::fnc1_first_portion:
                case mode::fnc1_second_portion:
                case mode::kanji:
                case mode::hanzi:
                case mode::structured_append:
                case mode::eci:
                    return {}; // no support (yet?)
                default: {
                    // "Normal" QR code modes:
                    // How many characters will follow, encoded in this mode?
                    auto count = bits.read_bits(get_character_count_bits(current_mode, version));
                    switch (current_mode) {
                        case mode::numeric:
                            if (!decode_numeric(bits, result, count))
                                return {};
                            break;
                        case mode::alphanumeric:
                            if (!decode_alphanumeric(bits, result, count))
                                return {};
                            break;
                        case mode::byte:
                            if (!decode_byte(bits, result, count))
                                return {};
                            break;
                        default:
                            // some weird value
                            return {};
                    }
                }
            }
        } while (current_mode != mode::terminator);

        return result;
    }
}