//
// Created by dcnick3 on 2/2/21.
//

#include <pqrs/qr_scanner.h>
#include <pqrs/qr_ecc_decoder.h>
#include <pqrs/qr_bitstream_reader.h>
#include <pqrs/qr_block_splitter.h>
#include <pqrs/qr_version_info.h>
#include <pqrs/qr_bit_decoder.h>
#include <pqrs/homography_dlt.h>
#include <pqrs/interpolation.h>
#include <pqrs/direction4.h>
#include <pqrs/util.h>
#include <pqrs/qr_grid_reader.h>

#include <utility>
#include <vector>
#include <map>

namespace pqrs {

    namespace {
        constexpr int max_version_qr = 40;

        struct finder_node;

        struct finder_edge {
            finder_node &a;
            finder_node &b;
            int a_idx;
            int b_idx;
            float score;

            finder_node &other(finder_node &one);
            int other_idx(finder_node &one);

            finder_edge(finder_node &a, finder_node &b, int a_idx, int b_idx, float score);
        };

        struct finder_node {
            finder_pattern _pattern;
            float _largest_side_size{-std::numeric_limits<float>::max()};
            float _smallest_side_size{std::numeric_limits<float>::max()};

            // index is for side index
            std::array<std::optional<std::shared_ptr<finder_edge>>, 4> _neighbours;

            explicit finder_node(finder_pattern pattern) : _pattern(std::move(pattern)) {
                for (int i = 0; i < 4; i++) {
                    auto[a, b] = pattern[i];
                    _largest_side_size = std::max(_largest_side_size, (float) (b - a).norm());
                    _smallest_side_size = std::min(_smallest_side_size, (float) (b - a).norm());
                }
            }

            void disconnect_edge(int index) {
                assert(_neighbours[index]);
                auto edge = *_neighbours[index]; // now we own it =)
                edge->a._neighbours[edge->a_idx] = {};
                edge->b._neighbours[edge->b_idx] = {};
            }
        };

        finder_node &pqrs::finder_edge::other(finder_node &one) {
            if (&one == &a)
                return b;
            return a;
        }

        int pqrs::finder_edge::other_idx(finder_node &one) {
            if (&one == &a)
                return b_idx;
            return a_idx;
        }

        pqrs::finder_edge::finder_edge(finder_node &a,
                                       finder_node &b,
                                       int a_idx,
                                       int b_idx,
                                       float score) : a(a), b(b),
                                                      a_idx(a_idx),
                                                      b_idx(b_idx),
                                                      score(score) {}

        void consider_connect(finder_node &a, finder_node &b) {

            auto line_a = a._pattern.center;
            auto line_b = b._pattern.center;

            auto intersection_a = tetragon_segment_intersection_any(a._pattern.poly, line_a, line_b);
            auto intersection_b = tetragon_segment_intersection_any(b._pattern.poly, line_a, line_b);
            if (!(intersection_a && intersection_b)) {
                assert("Intersection is fucked up" && 0);
                return;
            }

            auto[ia_idx, ia_pt] = *intersection_a;
            auto[ib_idx, ib_pt] = *intersection_b;

            auto[a0, a1] = a._pattern[ia_idx];
            auto[b0, b1] = b._pattern[ib_idx];

            auto side_a_length = (a0 - a1).norm();
            auto side_b_length = (b0 - b1).norm();

            auto side_loc_a = (ia_pt - a._pattern.poly[ia_idx]).norm() / side_a_length;
            auto side_loc_b = (ib_pt - b._pattern.poly[ib_idx]).norm() / side_b_length;

            if (std::abs(side_loc_a - .5f) > .35f
                || std::abs(side_loc_b - .5f) > .35f) {
                return;
            }

            // see if connecting sides are of similar size
            if (std::abs(side_a_length - side_b_length) / std::max(side_a_length, side_b_length) > .25f)
                return;

            auto angle = line_acute_angle(a0, a1, b0, b1);

            if (angle > M_PI_4)
                return;

            auto ratio = std::max(a._smallest_side_size / b._largest_side_size,
                                  b._smallest_side_size / a._largest_side_size);

            // TODO: Does this happen ever?
            if (ratio > 1.3f)
                return;

            auto score = (ia_pt - ib_pt).norm() * (1.f + angle / .1f);

            // connect them if they are better candidates
            if (a._neighbours[ia_idx] && a._neighbours[ia_idx]->get()->score > score) {
                a.disconnect_edge(ia_idx);
            }
            if (b._neighbours[ib_idx] && b._neighbours[ib_idx]->get()->score > score) {
                b.disconnect_edge(ib_idx);
            }
            if (!a._neighbours[ia_idx] && !b._neighbours[ib_idx]) {
                auto edge = std::make_shared<finder_edge>(a, b, ia_idx, ib_idx, score);
                a._neighbours[ia_idx] = edge;
                b._neighbours[ib_idx] = edge;
            }
        }

        struct reader {
            qr_grid_local _grid;
            dynamic_bitset& _bs;

        public:
            reader(qr_grid_local grid, dynamic_bitset &bs) : _grid(std::move(grid)), _bs(bs) {}

            void operator()(int x, int y) const {
                _bs.push_back(_grid.sample(x, y));
            }
        };

        typedef std::vector<std::pair<vector2d, vector2d>> homo_features;

        struct candidate_qr_code {
            finder_pattern origin;
            finder_pattern right;
            finder_pattern bottom;

            std::optional<qr_format> format;
            std::optional<int> version;
            std::optional<std::vector<std::pair<vector2d, vector2d>>> alignment_patterns;

            candidate_qr_code(finder_pattern origin, finder_pattern right, finder_pattern bottom)
                    : origin(std::move(origin)), right(std::move(right)), bottom(std::move(bottom)) {}

            [[nodiscard]] dynamic_bitset read_format_region_0(gray_u8 const& image) const {
                auto grid = qr_grid_local::from_finder(image, origin);
                dynamic_bitset r;
                r.reserve(15);

                reader read(grid, r);

                for (int i = 0; i < 6; i++) {
                    read(8, i);
                }

                read(8, 7);
                read(8, 8);
                read(7, 8);

                for (int i = 0; i < 6; i++) {
                    read(5 - i, 8);
                }

                return r;

            }

            [[nodiscard]] dynamic_bitset read_version_region_0(gray_u8 const& image) const {
                auto grid = qr_grid_local::from_finder(image, right);
                dynamic_bitset r;
                r.reserve(18);

                reader read(grid, r);

                for (int i = 0; i < 18; i++) {
                    int row = i/3;
                    int col = i%3;
                    read(col - 4, row);
                }

                return r;

            }

            [[nodiscard]] dynamic_bitset read_format_region_1(gray_u8 const& image) const {
                dynamic_bitset r;
                r.reserve(15);

                {
                    reader read(qr_grid_local::from_finder(image, right), r);

                    for (int i = 0; i < 8; i++) {
                        read(6 - i, 8);
                    }
                }


                {
                    reader read(qr_grid_local::from_finder(image, bottom), r);

                    for (int i = 0; i < 7; i++) {
                        read(8, i);
                    }
                }

                return r;

            }

            [[nodiscard]] dynamic_bitset read_version_region_1(gray_u8 const& image) const {
                dynamic_bitset r;
                r.reserve(18);

                {
                    reader read(qr_grid_local::from_finder(image, bottom), r);

                    for (int i = 0; i < 18; i++) {
                        int row = i%3;
                        int col = i/3;
                        read(col, row - 4);
                    }
                }

                return r;

            }

            bool try_determine_format(gray_u8 const& image) {
                auto zero = read_format_region_0(image);
                auto zero_format = try_decode_qr_format(zero);
                if (zero_format) {
                    format = zero_format;
                    return true;
                }
                format = try_decode_qr_format(read_format_region_1(image));
                return bool(format);
            }

            std::optional<int> estimate_version_by_size() {
                // here we want to estimate the grid coordinates of finder pattern positions on image
                // we don't (yet) know the right & bottom marker grid coordinates, so we only put lines to homography
                std::vector<std::pair<homo_vector2d, homo_vector2d>> pps;

                // points
                pps.push_back({homo_vector2d(origin.poly[0]), {7, 0, 1}});
                pps.push_back({homo_vector2d(origin.poly[1]), {7, 7, 1}});
                pps.push_back({homo_vector2d(origin.poly[2]), {0, 7, 1}});
                //pps.push_back({homo_vector2d(origin.poly[3]), {0, 0, 1}});

                auto line = [&](vector2d v1, vector2d v2) -> homo_vector2d {
                    auto v = (v2 - v1).normal();
                    return {v.x(), v.y(), 0};
                };

                // line directions (aka points at infinity)
                // horizontal
                pps.push_back({line(origin.poly[0], right.poly[3]), {1, 0, 0}});
                pps.push_back({line(origin.poly[1], right.poly[2]), {1, 0, 0}});
                // vertical
                pps.push_back({line(origin.poly[2], bottom.poly[3]), {0, 1, 0}});
                pps.push_back({line(origin.poly[1], bottom.poly[0]), {0, 1, 0}});

                auto homo = estimate_homography(pps);

                auto right_grid = homo.map(right.poly[3]);
                auto bottom_grid = homo.map(bottom.poly[3]);

                auto version_x = ((right_grid.x() + 7) - 17) / 4;
                auto version_y = ((bottom_grid.y() + 7) - 17) / 4;

                if (std::abs(right_grid.y() / right_grid.x()) >= .3f)
                    return {};
                if (std::abs(bottom_grid.x() / bottom_grid.y()) >= .3f)
                    return {};

                if (std::abs(version_x - version_y) / std::max(version_x, version_y) > 0.4)
                    return {};

                return std::lround((version_x + version_y) / 2.0);
            }

            bool try_determine_version(gray_u8 const& image) {
                auto estimate = estimate_version_by_size();
                if (!estimate)
                    return false;
                if (*estimate < 7) {
                    if (estimate < 1)
                        return false;
                    version = estimate;
                    return true;
                } else {
                    auto v0 = try_decode_qr_version(read_version_region_0(image));
                    auto v1 = try_decode_qr_version(read_version_region_1(image));
                    if (!v0 && !v1) return false;
                    if (!v0 != !v1) {
                        version = v0 ? v0 : v1;
                        return true;
                    }
                    if (*v0 != *v1)
                        return false;
                    version = *v0;
                    return true;
                }
            }

            [[nodiscard]] homo_features make_homography_features() const {
                homo_features pts;

                auto module_size = 17.f + (float)*version * 4;

                pts.push_back({{7, 0}, origin.poly[0]});
                pts.push_back({{7, 7}, origin.poly[1]});
                pts.push_back({{0, 7}, origin.poly[2]});

                pts.push_back({{module_size, 7}, right.poly[1]});
                pts.push_back({{module_size - 7, 7}, right.poly[2]});
                pts.push_back({{module_size - 7, 0}, right.poly[3]});

                pts.push_back({{7, module_size - 7}, bottom.poly[0]});
                pts.push_back({{7, module_size}, bottom.poly[1]});
                pts.push_back({{0, module_size - 7}, bottom.poly[3]});

                if (alignment_patterns)
                    pts.insert(pts.end(), alignment_patterns->begin(), alignment_patterns->end());

                return std::move(pts);
            }

            [[nodiscard]] bool try_find_alignment_patterns(gray_u8 const& image) {
                auto const& version_info = get_version_info(*version);

                auto homo_features = make_homography_features();
                auto homo = estimate_homography(homo_features);
                auto homo_inv = homo.inverse();

                auto orig_pos_grid = version_info.alignment_grid();
                //xt::xtensor<std::optional<vector2d>, 2> corrected_pos_grid(orig_pos_grid.shape(),
                //                                                           std::optional<vector2d>());

                std::vector<std::pair<vector2d, vector2d>> res;

                auto center_on_square = [&](vector2d p) -> vector2d {
                    auto step = 1.f;
                    auto best_mag = std::numeric_limits<float>::max();
                    auto best_p = p;

                    xt::xtensor_fixed<float, xt::xshape<3 ,3>> samples;

                    for (int i = 0; i < 10; i++) {
                        for (int row = 0; row < 3; row++) {
                            auto grid_y = p.y() - 1.f + (float)row;
                            for (int col = 0; col < 3; col++) {
                                auto grid_x = p.x() - 1.f + (float)col;

                                auto sample_point = homo.map({grid_x, grid_y});

                                samples(row, col) = interpolate_bilinear(image, sample_point);
                            }
                        }

                        auto dx = (samples(0, 2)+samples(1, 2)+samples(2, 2))-(samples(0, 0)+samples(1, 0)+samples(2, 0));
                        auto dy = (samples(2, 0)+samples(2, 1)+samples(2, 2))-(samples(0, 0)+samples(0, 1)+samples(0, 2));

                        vector2d dp(dx, dy);
                        auto r = dp.norm();
                        dp /= r;

                        if (best_mag > r) {
                            best_mag = r;
                            best_p = p;
                        } else
                            step *= .75f;

                        if (r > 0) {
                            p += dp * step;
                        } else
                            break;
                    }

                    return best_p;
                };

                for (int row = 0; row < orig_pos_grid.shape(0); row++) {
                    for (int col = 0; col < orig_pos_grid.shape(1); col++) {
                        auto const& orig = orig_pos_grid(row, col);

                        if (!orig)
                            continue;

                        // those are not yet implemented, as they are only useful for bit QRs
                        float adj_y = .0f, adj_x = .0f;

                        vector2d p(*orig);
                        p.x() += .5f + adj_x;
                        p.y() += .5f + adj_y;

                        p = center_on_square(p);

                        p -= vector2d(.5f, .5f);

                        auto orig_image_p = homo.map(vector2d(*orig));
                        auto image_p = homo.map(p);

                        res.emplace_back(vector2d(*orig), image_p);
                    }
                }

                alignment_patterns = std::move(res);

                return true;
            }
        };

        finder_pattern shift_finder(finder_pattern p, int src_direction, direction4 target_direction) {
            while (direction4(src_direction) != target_direction) {
                auto p1 = p;
                p.poly[0] = p1.poly[1];
                p.poly[1] = p1.poly[2];
                p.poly[2] = p1.poly[3];
                p.poly[3] = p1.poly[0];

                src_direction = (src_direction + 3) % 4;
            }
            return p;
        }

        std::vector<candidate_qr_code> find_candidates(std::vector<finder_pattern> const& finder_patterns) {
            if (finder_patterns.size() < 3)
                return {};
            // firstly we need to group finder patterns
            // firstly, we group them by distance. This is part of NN problem, but we don't have many points, so O(n^2) algo would do
            std::vector<finder_node> nodes;

            nodes.reserve(finder_patterns.size());
            for (auto const& p : finder_patterns)
                nodes.emplace_back(p);

            // compute graph of candidate position pattern groups

            // TODO: test this on multiple qr codes

            for (auto& node : nodes) {
                // The QR code version specifies the number of "modules"/blocks across the marker is
                // A position pattern is 7 blocks. A version 1 qr code is 21 blocks. Each version past one increments
                // by 4 blocks. The search is relative to the center of each position pattern, hence the - 7
                auto maximum_qr_code_width = node._largest_side_size * (17 + 4 * max_version_qr - 7) / 7.f;
                auto search_radius = 1.2f * maximum_qr_code_width;
                search_radius *= search_radius;

                for (auto& node1 : nodes) {
                    if (&node != &node1) {
                        if ((node1._pattern.center - node._pattern.center).norm_squared() <= search_radius)
                            consider_connect(node, node1);
                    }
                }
            }

            // now find candidate qr codes

            std::vector<candidate_qr_code> candidates;

            for (auto& node : nodes) {
                for (int i = 0; i < 4; i++) {
                    int j = (i + 1) % 4;
                    if (node._neighbours[i] && node._neighbours[j]) {
                        auto origin = shift_finder(node._pattern, i, direction4::right);
                        auto r_edge = node._neighbours[i]->get();
                        auto b_edge = node._neighbours[j]->get();
                        auto right = shift_finder(r_edge->other(node)._pattern, r_edge->other_idx(node), direction4::left);
                        auto bottom = shift_finder(b_edge->other(node)._pattern, b_edge->other_idx(node), direction4::top);

                        candidates.emplace_back(origin, right, bottom);
                    }
                }
            }

            return candidates;
        }

        [[nodiscard]] qr_grid_global make_grid(gray_u8 const& image, homo_features const& features,
                                        std::uint8_t threshold, int size) {
            auto homo = estimate_homography(features);
            return qr_grid_global(image, homo, size);
        }

        [[nodiscard]] std::optional<std::string> try_decode(const gray_u8 &image, const detected_qr &detected_qr) {

            auto version = detected_qr._version;
            auto format = detected_qr._format;
            auto error_level = detected_qr._format._error_level;
            auto features = detected_qr._homography_features;
            auto threshold = detected_qr._threshold;
            auto grid = make_grid(image, features, threshold, detected_qr.size());

            auto raw_data = read_raw_data(version, format, [&](point2d p) {
                return grid.sample(p);
            });

            auto const& version_info = get_version_info(version);
            auto const& level_info = version_info.ec_blocks_for_level(error_level);

            auto blocks = split_blocks(raw_data, version, error_level);

            std::vector<uint8_t> corrected_data;
            corrected_data.reserve(level_info.get_total_data_codewords());

            bool err = false;
            for (auto& block : blocks) {
                if (!correct_qr_errors(block)) {
                    err = true;
                    break;
                }
                corrected_data.insert(corrected_data.end(), block._data_and_ecc.begin(),
                                      block._data_and_ecc.begin() + block._data_size);
            }

            if (err)
                return {};

            auto str = decode_bits(corrected_data, version);

            return str;
        }
    }


    std::vector<detected_qr> detect_qr_codes(gray_u8 const& image,
                                             std::vector<finder_pattern> const& finder_patterns) {

        auto candidates = find_candidates(finder_patterns);

        std::vector<detected_qr> res;

        for (auto& candidate : candidates) {
            if (!candidate.try_determine_format(image))
                continue;

            if (!candidate.try_determine_version(image))
                continue;

            if (!candidate.try_find_alignment_patterns(image))
                continue;

            auto homo_features = candidate.make_homography_features();
            auto thresh = (candidate.bottom.gray_threshold +
                    candidate.right.gray_threshold + candidate.origin.gray_threshold) / 3;
            res.emplace_back(*candidate.version, *candidate.format, homo_features, thresh);
        }

        return res;
    }

    std::optional<decoded_qr> decode_qr_code(const gray_u8 &image, detected_qr detected_qr) {

        auto& version = detected_qr._version;
        auto& format = detected_qr._format;
        auto& error_level = detected_qr._format._error_level;
        auto& features = detected_qr._homography_features;
        auto& threshold = detected_qr._threshold;

        for (int attempt = 0; attempt < 6; attempt++) {
            auto str = try_decode(image, detected_qr);

            if (str)
                return {{version, format, std::move(features), threshold, *str}};

            // No? Remove feature with the largest error and try again

            auto homo_inv = detected_qr._homography.inverse();

            int selected = -1;
            auto largest_error = .0f;

            for (int i = 0; i < features.size(); i++) {
                auto mapped = homo_inv.map(features[i].second);
                auto error = (mapped - features[i].first).norm_squared();
                if (error > largest_error) {
                    largest_error = error;
                    selected = i;
                }
            }

            if (selected == -1)
                return {};
            features.erase(features.begin() + selected);
            detected_qr._homography = estimate_homography(features);
        }

        return {};
    }

    /*
    std::vector<decoded_qr> scan_qr_codes(gray_u8 const& image,
                                          std::vector<finder_pattern> const& finder_patterns) {

        auto candidates = find_candidates(finder_patterns);

        std::vector<decoded_qr> res;

        for (auto& candidate : candidates) {
            if (!candidate.try_determine_format(image))
                continue;

            if (!candidate.try_determine_version(image))
                continue;

            if (!candidate.try_find_alignment_patterns(image))
                continue;

            // TODO: detect alignment patterns to add more features for homography

            auto homo_features = candidate.make_homography_features();

            auto grid = candidate.make_grid(image, homo_features);

            auto raw_data = read_raw_data(*candidate.version, *candidate.format, [&](point2d p) {
                return grid.sample((float)p.x() + .5f , (float)p.y() + .5f);
            });

            auto const& version_info = get_version_info(*candidate.version);
            auto const& level_info = version_info.ec_blocks_for_level(candidate.format->_error_level);

            auto blocks = split_blocks(raw_data, *candidate.version, candidate.format->_error_level);

            std::vector<uint8_t> corrected_data;
            corrected_data.reserve(level_info.get_total_data_codewords());

            bool err = false;
            for (auto& block : blocks) {
                if (!correct_qr_errors(block)) {
                    err = true;
                    break;
                }
                corrected_data.insert(corrected_data.end(), block._data_and_ecc.begin(),
                                      block._data_and_ecc.begin() + block._data_size);
            }

            if (!err) {
                auto str = decode_bits(corrected_data, *candidate.version);

                if (str) {
                    res.emplace_back(*candidate.version, *candidate.format, grid._homography, *str);
                    continue;
                }
            }

            // put the result, but without the decoded result (that failed)
            res.emplace_back(*candidate.version, *candidate.format, grid._homography);
        }

        return res;
    }
     */
}
