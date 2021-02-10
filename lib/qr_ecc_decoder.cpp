//
// Created by dcnick3 on 2/1/21.
//

#include <pqrs/qr_ecc_decoder.h>
#include <pqrs/util.h>
#include <pqrs/qr_version_info.h>

#include <cstdint>
#include <array>
#include <stdexcept>
#include <optional>
#include <algorithm>
#include <utility>
#include <iostream>

// this one is much more inspired by zxing's ReedSolomonDecoder

namespace pqrs {

    namespace {
        constexpr std::uint32_t qr_code_data_generator = 0b100011101;
        constexpr std::uint32_t qr_code_format_generator = 0b10100110111;
        constexpr std::uint32_t qr_code_version_generator = 0b1111100100101;
        constexpr std::uint32_t qr_code_format_mask = 0b101010000010010;

        template<int N, std::uint32_t primitive>
        struct gf2n {
            static constexpr std::uint32_t alpha = 2;
            static constexpr std::uint32_t num_values = 1U << N;
            static constexpr std::uint32_t max_value = num_values - 1;

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

            static inline std::uint32_t add(std::uint32_t x, std::uint32_t y) {
                return x ^ y;
            }

            static inline std::uint32_t sub(std::uint32_t x, std::uint32_t y) {
                return x ^ y;
            }

            static inline std::uint32_t mul(std::uint32_t x, std::uint32_t y) {
                if (x == 0 || y == 0)
                    return 0;
                return exp_table[log_table[x] + log_table[y]];
            }

            static inline std::uint32_t div(std::uint32_t x, std::uint32_t y) {
                if (y == 0)
                    throw std::domain_error("Division by zero");
                if (x == 0)
                    return 0;

                return exp_table[log_table[x] + max_value - log_table[y]];
            }

            static inline std::uint32_t pow(std::uint32_t x, std::uint32_t power) {
                return exp_table[(log_table[x] * power) % max_value];
            }

            static inline std::uint32_t log(std::uint32_t x) {
                return log_table[x];
            }

            static inline std::uint32_t inv(std::uint32_t x) {
                return exp_table[max_value - log_table[x]];;
            }
        };


        template<typename GF>
        class gf_el {
            std::uint32_t _value;
        public:
            gf_el() : _value(0) {}
            explicit inline gf_el(std::uint32_t value) : _value(value) {
                assert(_value >= 0 && value <= GF::max_value);
            }

            static constexpr gf_el zero() { return gf_el(0); }
            static constexpr gf_el one() { return gf_el(1); }
            static constexpr gf_el alpha() { return gf_el(GF::alpha); }

            [[nodiscard]] inline std::uint32_t value() const { return _value; }
            inline gf_el operator+(gf_el o) const { return gf_el(GF::add(_value, o._value)); }
            inline gf_el operator-(gf_el o) const { return gf_el(GF::sub(_value, o._value)); }
            inline gf_el operator*(gf_el o) const { return gf_el(GF::mul(_value, o._value)); }
            inline gf_el operator/(gf_el o) const { return gf_el(GF::div(_value, o._value)); }
            inline gf_el pow(gf_el power) const { return gf_el(GF::pow(_value, power._value)); }
            inline gf_el inv() const { return gf_el(GF::inv(_value)); }
            inline gf_el log() const { return gf_el(GF::log(_value)); }

            inline gf_el& operator+=(gf_el o) { return *this = *this + o; }
            inline gf_el& operator-=(gf_el o) { return *this = *this - o; }
            inline gf_el& operator*=(gf_el o) { return *this = *this * o; }
            inline gf_el& operator/=(gf_el o) { return *this = *this / o; }

            inline bool operator==(gf_el o) const { return _value == o._value; }
            inline bool operator!=(gf_el o) const { return _value != o._value; }
        };

        template<typename EL>
        class poly {
            std::vector<EL> _elements;
            inline explicit poly(std::size_t sz) : _elements(sz) {
                assert(sz > 0);
            }
            [[nodiscard]] std::size_t size() const { return _elements.size(); }

            void normalize() {
                // not very effective
                while (_elements.size() > 1 && _elements[0] == EL::zero()) {
                    _elements.erase(_elements.begin());
                }
            }

        public:

            [[nodiscard]] std::size_t degree() const { return _elements.size() - 1; }

            static constexpr poly zero() { return poly({EL::zero()}); }
            static constexpr poly one() { return poly({EL::one()}); }

            inline explicit poly(std::vector<EL> elements) : _elements(std::move(elements)) {
                assert(!_elements.empty());
                normalize();
            }

            [[nodiscard]] inline poly operator*(EL a) const {
                poly res(_elements);
                std::transform(res._elements.begin(), res._elements.end(), res._elements.begin(), [&](auto el) {
                    return el * a;
                });
                res.normalize();
                return res;
            }

            /*
             * Add two polynomials
             */
            [[nodiscard]] inline poly operator+(poly const& o) const {
                poly res(std::max(size(), o.size()));
                for (int i = 0; i < size(); i++)
                    res._elements[i + res.size() - size()] += _elements[i];
                for (int i = 0; i < o.size(); i++)
                    res._elements[i + res.size() - o.size()] += o._elements[i];
                res.normalize();
                return res;
            }

            /*
             * Subtract two polynomials
             */
            [[nodiscard]] inline poly operator-(poly const& o) const {
                poly res(std::max(size(), o.size()));
                for (int i = 0; i < size(); i++)
                    res._elements[i + res.size() - size()] += _elements[i];
                for (int i = 0; i < o.size(); i++)
                    res._elements[i + res.size() - o.size()] -= o._elements[i];
                res.normalize();
                return res;
            }

            /*
             * Multiply two polynomials, inside Galois Field
             */
            [[nodiscard]] inline poly operator*(poly const& o) const {
                poly res(size() + o.size() - 1);
                for (int i = 0; i < size(); i++)
                    for (int j = 0; j < o.size(); j++) {
                        res._elements[i + j] += _elements[i] * o._elements[j];
                    }
                res.normalize();
                return res;
            }

            /*
             * Evaluates a polynomial in GF(2^p) given the value for x.
             * This is based on Horner's scheme for maximum efficiency.
             */
            [[nodiscard]] inline EL operator()(EL x) const {
                auto y = _elements[0];
                for (int i = 1; i < size(); i++) {
                    y = (y * x) + _elements[i];
                }
                return y;
            }

            /*
             * Fast polynomial division by using Extended Synthetic Division and optimized for GF(2^p) computations
             * (doesn't work with standard polynomials outside of this galois field,
             * see the Wikipedia article for generic algorithm).
             */
            [[nodiscard]] inline std::pair<poly, poly> divide(poly const& divisor) const {
                if (divisor.size() > size())
                    return std::make_pair(zero(), *this);

                poly quotient(*this);

                auto normalizer = divisor._elements[0];

                auto N = size() - divisor.size() + 1;
                for (int i = 0; i < N; i++) {
                    quotient._elements[i] /= normalizer;

                    auto coef = quotient._elements[i];
                    if (coef != EL::zero()) { // division by zero is undefined.
                        for (int j = 1; j < divisor.size(); j++) {
                            auto div_j = divisor._elements[j];

                            if (div_j != EL::zero()) // log(0) is undefined.
                                quotient._elements[i + j] -= div_j * coef;
                        }
                    }
                }
                poly remainder(divisor.size() - 1);
                std::copy(quotient._elements.begin() + quotient.size() - remainder.size(),
                          quotient._elements.begin() + remainder.size(),
                          remainder._elements.begin());
                quotient._elements.resize(quotient.size() - remainder.size());

                quotient.normalize();
                remainder.normalize();

                return std::make_pair(quotient, remainder);
            }

            [[nodiscard]] inline EL operator[](int index) const { return _elements[index]; }

            [[nodiscard]] inline poly operator/(poly const& o) const { return divide(o).first; }
            [[nodiscard]] inline poly operator%(poly const& o) const { return divide(o).second; }

            [[nodiscard]] inline EL get_coefficient(int degree) const {
                return _elements[_elements.size() - 1 - degree];
            }

            [[nodiscard]] inline bool is_zero() const {
                return std::all_of(_elements.begin(), _elements.end(), [](auto a){ return a == EL::zero(); });
            }

            static inline poly generator(int nsym) {
                poly res = one();
                for (int i = 0; i < nsym; i++) {
                    res = res * poly({1, EL::alpha().pow(EL(i)) });
                }
                res.normalize();
                return res;
            }

            static inline poly monomial(int degree, EL coefficient) {
                assert(degree >= 0);
                if (coefficient == EL::zero())
                    return zero();
                std::vector<EL> coef_vect(degree + 1);
                coef_vect[0] = coefficient;
                return poly(std::move(coef_vect));
            }

            static inline std::optional<std::pair<poly, poly>> euclidean_algorithm(poly a, poly b, int R) {
                if (a.degree() < b.degree())
                    std::swap(a, b);

                poly r_last = a;
                poly r = b;
                poly t_last = poly::zero();
                poly t = poly::one();

                // Run Euclidean algorithm until r's degree is less than R/2
                while (r.degree() >= R / 2) {
                    auto r_last_last = r_last;
                    auto t_last_last = t_last;
                    r_last = r;
                    t_last = t;

                    // Divide r_last_last by r_last, with quotient in q and remainder in r
                    if (r_last.is_zero()) {
                        // Oops, Euclidean algorithm already terminated?
                        return {};
                        //throw new ReedSolomonException("r_{i-1} was zero");
                    }
                    r = r_last_last;
                    poly q = zero();
                    auto denominator_leading_term = r_last.get_coefficient(r_last.degree());
                    auto dlt_inverse = denominator_leading_term.inv();
                    while (r.degree() >= r_last.degree() && !r.is_zero()) {
                        int degree_diff = r.degree() - r_last.degree();
                        auto scale = r.get_coefficient(r.degree()) * dlt_inverse;
                        q = q + monomial(degree_diff, scale);
                        r = r + r_last * monomial(degree_diff, scale);
                    }

                    t = q * t_last + t_last_last;

                    if (r.degree() >= r_last.degree()) {
                        std::terminate();
                        //throw new IllegalStateException("Division algorithm failed to reduce polynomial?");
                    }
                }

                auto sigma_tilde_at_zero = t.get_coefficient(0);
                if (sigma_tilde_at_zero == EL::zero()) {
                    return {};
                    //throw new ReedSolomonException("sigmaTilde(0) was zero");
                }

                auto inverse = sigma_tilde_at_zero.inv();
                auto sigma = t * inverse;
                auto omega = r * inverse;
                return {{sigma, omega}};
            }
        };

        template struct gf2n<8, qr_code_data_generator>;
        typedef gf2n<8, qr_code_data_generator> qr_gf;

        template struct gf_el<qr_gf>;
        typedef gf_el<qr_gf> qr_gf_el;

        template struct poly<qr_gf_el>;
        typedef poly<qr_gf_el> qr_gf_el_poly;


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

        std::optional<std::vector<qr_gf_el>> find_error_locations(qr_gf_el_poly const& error_locator) {
            auto num_errors = error_locator.degree();
            if (num_errors == 1) {
                return {{ error_locator.get_coefficient(1) }};
            }
            std::vector<qr_gf_el> result(num_errors);
            int e = 0;
            for (int i = 1; i < 256 && e < num_errors; i++) {
                if (error_locator(qr_gf_el(i)) == qr_gf_el::zero()) {
                    result[e] = qr_gf_el(i).inv();
                    e++;
                }
            }
            if (e != num_errors) {
                return {};
                //throw new ReedSolomonException("Error locator degree does not match number of roots");
            }
            return result;
        }

        std::optional<std::vector<qr_gf_el>> find_error_magnitudes(qr_gf_el_poly const& error_evaluator,
                                                              std::vector<qr_gf_el> const& error_locations) {
            // This is directly applying Forney's Formula
            auto s = error_locations.size();
            std::vector<qr_gf_el> result(s);
            for (int i = 0; i < s; i++) {
                auto xiInverse = error_locations[i].inv();
                auto denominator = qr_gf_el::one();
                for (int j = 0; j < s; j++) {
                    if (i != j)
                        denominator = denominator * (error_locations[j] * xiInverse + qr_gf_el::one());
                }
                result[i] = error_evaluator(xiInverse) * denominator.inv();
            }
            return result;
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

    bool correct_qr_errors(qr_block &block) {
        auto two_s = block._data_and_ecc.size() - block._data_size;

        std::vector<qr_gf_el> poly_data;
        poly_data.reserve(block._data_and_ecc.size());
        for (auto e : block._data_and_ecc)
            poly_data.emplace_back(e);


        auto no_error = true;
        std::vector<qr_gf_el> syndrome_coefficients(two_s);
        {
            qr_gf_el_poly poly(poly_data);
            for (int i = 0; i < syndrome_coefficients.size(); i++) {
                auto eval = poly(qr_gf_el::alpha().pow(qr_gf_el(i)));
                if (eval != qr_gf_el::zero()) {
                    no_error = false;
                }
                *(syndrome_coefficients.rbegin() + i) = eval;
            }
        }

        if (no_error)
            return true;

        qr_gf_el_poly syndrome_poly(std::move(syndrome_coefficients));

        auto res = qr_gf_el_poly::euclidean_algorithm(qr_gf_el_poly::monomial(two_s, qr_gf_el::one()), syndrome_poly, two_s);
        if (!res)
            return false;
        auto [sigma, omega] = *res;
        auto error_locations_opt = find_error_locations(sigma);
        if (!error_locations_opt)
            return false;
        auto error_magnitudes_opt = find_error_magnitudes(omega, *error_locations_opt);
        if (!error_magnitudes_opt)
            return false;

        for (int i = 0; i < error_locations_opt->size(); i++) {
            int position = poly_data.size() - 1 - (*error_locations_opt)[i].log().value();
            if (position < 0)
                // throw new ReedSolomonException("Bad error location");
                return false;
            poly_data[position] -= (*error_magnitudes_opt)[i];
        }

        for (int i = 0; i < block._data_size; i++) {
            block._data_and_ecc[i] = poly_data[i].value();
        }

        return true;
    }

}