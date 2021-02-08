//
// Created by dcnick3 on 2/1/21.
//

#include <pqrs/qr_decoder.h>
#include <pqrs/util.h>
#include <pqrs/qr_version_info.h>

#include <cstdint>
#include <array>
#include <stdexcept>
#include <optional>

namespace pqrs {

    namespace {
        constexpr std::uint32_t qr_code_data_generator = 0b100011101;
        constexpr std::uint32_t qr_code_format_generator = 0b10100110111;
        constexpr std::uint32_t qr_code_version_generator = 0b1111100100101;
        constexpr std::uint32_t qr_code_format_mask = 0b101010000010010;

        template<int N, std::uint32_t primitive>
        struct gf2n {
            static constexpr std::uint32_t num_values = 1U << N;
            static constexpr std::uint32_t max_value = num_values - 1;

            inline static constexpr std::uint32_t add(std::uint32_t a, std::uint32_t b) {
                return a ^ b;
            }

        private:
            /**
             * <p>Multiply the two polynomials together. The technique used here isn't the fastest but is easy
             * to understand.</p>
             *
             * <p>NOTE: No modulus operation is performed so the result might not be a member of the same field.</p>
             *
             * @param a polynomial
             * @param b polynomial
             * @return result polynomial
             */
            inline static constexpr std::uint32_t mul_raw(std::uint32_t a, std::uint32_t b) {
                std::uint32_t z = 0;

                for (int i = 0; (b >> i) > 0; i++)
                    if ((b & (1 << i)) != 0)
                        z ^= a << i;

                return z;
            }

            /**
             * Implementation of multiplication with a primitive polynomial. The result will be a member of the same field
             * as the inputs, provided primitive is an appropriate irreducible polynomial for that field.
             *
             * Uses 'Russian Peasant Multiplication' that should be a faster algorithm.
             *
             * @param a polynomial
             * @param b polynomial
             * @param primitive Primitive polynomial which is irreducible.
             * @param domain Value of a the largest possible value plus 1. E.g. GF(2**8) would be 256
             * @return result polynomial
             */
            inline static constexpr std::uint32_t mul_raw(std::uint32_t a, std::uint32_t b, std::uint32_t domain) {
                std::uint32_t r = 0;
                while (b > 0) {
                    if ((b & 1) != 0)
                        r = r ^ a;
                    b = b >> 1;
                    a = a << 1;

                    if (a >= domain)
                        a ^= primitive;
                }
                return r;
            }

            static constexpr std::array<std::uint32_t, num_values> compute_log_table() {
                std::array<std::uint32_t, num_values> log{};

                std::uint32_t x = 1;
                for (int i = 0; i < max_value; i++) {
                    log[x] = i;
                    x = mul_raw(x, 2, num_values);
                }

                return log;
            }

            static constexpr std::array<std::uint32_t, num_values * 2> compute_exp_table() {
                std::array<std::uint32_t, num_values * 2> exp{};

                std::uint32_t x = 1;
                for (int i = 0; i < max_value; i++) {
                    exp[i] = x;
                    x = mul_raw(x, 2, num_values);
                }

                for (int i = 0; i < num_values; i++) {
                    exp[i + max_value] = exp[i];
                }

                return exp;
            }


        public:
            static constexpr std::array<std::uint32_t, num_values> log_table = compute_log_table();
            static constexpr std::array<std::uint32_t, num_values * 2> exp_table = compute_exp_table();

            static inline std::uint32_t mul(std::uint32_t x, std::uint32_t y) {
                if (x == 0 || y == 0)
                    return 0;
                return exp_table[log_table[x] + log_table[y]];
            }

            static inline std::uint32_t divide(std::uint32_t x, std::uint32_t y) {
                if (y == 0)
                    throw std::domain_error("Division by zero");
                if (x == 0)
                    return 0;

                return exp_table[log_table[x] + max_value - log_table[y]];
            }

            static inline std::uint32_t power(std::uint32_t x, std::uint32_t power) {
                return exp_table[(log_table[x] * power) % max_value];
            }

            static inline std::uint32_t inverse(std::uint32_t x) {
                return exp_table[max_value - log_table[x]];;
            }
        };

        template struct gf2n<8, qr_code_data_generator>;
        typedef gf2n<8, qr_code_data_generator> qr_gf;

        /**
         * Performs division using xcr operators on the encoded polynomials. used in BCH encoding/decoding
         * @param data Data being checked
         * @param generator Generator polynomial
         * @param total_bits Total number of bits in data
         * @param data_bits Number of data bits. Rest are error correction bits
         * @return Remainder after polynomial division
         */
        inline std::uint32_t
        bit_poly_modulus(std::uint32_t data, std::uint32_t generator, int total_bits, int data_bits) {
            int error_bits = total_bits - data_bits;
            for (int i = data_bits - 1; i >= 0; i--) {
                if ((data & (1 << (i + error_bits))) != 0)
                    data ^= generator << i;
            }
            return data;
        }

        inline std::optional<std::uint32_t>
        correct_dch(int N, std::uint32_t message, std::uint32_t generator, int total_bits, int data_bits) {
            int best_hamming = 255;
            std::optional<std::uint32_t> best_message{};

            int error_bits = total_bits - data_bits;

            // exhaustively check all possibilities
            for (int i = 0; i < N; i++) {
                std::uint32_t test = i << error_bits;
                test = test ^ bit_poly_modulus(test, generator, total_bits, data_bits);

                int distance = count_ones(test ^ message);

                // see if it found a better match
                if (distance < best_hamming) {
                    best_hamming = distance;
                    best_message = i;
                } else if (distance == best_hamming) {
                    // ambiguous so reject
                    best_message = {};
                }
            }
            return best_message;
        }
    }

    std::optional<qr_format> try_decode_qr_format(dynamic_bitset const& bits) {
        assert(bits.size() == 15);

        std::optional<std::uint32_t> message = *bits.data();

        *message ^= qr_code_format_mask;

        if (bit_poly_modulus(*message, qr_code_format_generator, 15, 5) == 0) {
            *message >>= 10U;
        } else {
            message = correct_dch(32, *message, qr_code_format_generator, 15, 5);
        }

        if (!message)
            return {};

        qr_format res{};
        res._mask_type = static_cast<mask_type>(*message & 0x7);
        res._error_level = static_cast<error_level>((*message >> 3) & 0x3);
        return res;
    }

    std::optional<int> try_decode_qr_version(dynamic_bitset const& bits) {
        assert(bits.size() == 18);

        std::optional<std::uint32_t> message = *bits.data();

        if (bit_poly_modulus(*message, qr_code_version_generator, 18, 6) == 0) {
            *message >>= 12U;
        } else {
            message = correct_dch(64, *message, qr_code_version_generator, 18, 6);
        }

        if (!message)
            return {};

        if (*message < 7 || *message > 40)
            return {};

        return *message;
    }

}