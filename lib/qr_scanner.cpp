//
// Created by dcnick3 on 2/2/21.
//

#include <pqrs/qr_scanner.h>
#include <pqrs/qr_ecc_decoder.h>
#include <pqrs/qr_bitstream_reader.h>
#include <pqrs/qr_block_splitter.h>
#include <pqrs/qr_version_info.h>
#include <pqrs/homography_dlt.h>
#include <pqrs/interpolation.h>
#include <pqrs/direction4.h>
#include <pqrs/util.h>

#include <utility>
#include <vector>
#include <map>

namespace pqrs {

    namespace {
        constexpr int max_version_qr = 40;

        struct qr_grid {
            gray_u8 const& _image;
            std::uint8_t _threshold;
            homography _homography;
        public:
            qr_grid(gray_u8 const& image, uint8_t threshold, homography homography)
                    : _image(image), _threshold(threshold), _homography(std::move(homography)) {}

            [[nodiscard]] bool sample(vector2d p) const {
                p = _homography.map(p);
                auto v = interpolate_bilinear(_image, p);
                return v < (float) _threshold;
            }

            [[nodiscard]] bool sample(float x, float y) const {
                return sample({x, y});
            }

            static qr_grid from_finder(gray_u8 const& image, finder_pattern const& finder) {
                std::vector<std::pair<vector2d, vector2d>> pts;

                pts.push_back({{7, 0}, finder.poly[0]});
                pts.push_back({{7, 7}, finder.poly[1]});
                pts.push_back({{0, 7}, finder.poly[2]});
                pts.push_back({{0, 0}, finder.poly[3]});

                auto homo = estimate_homography(pts);

                return qr_grid(image, finder.gray_threshold, homo);
            }
        };

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
            qr_grid const& _grid;
            dynamic_bitset& _bs;

        public:
            reader(qr_grid const& grid, dynamic_bitset &bs) : _grid(grid), _bs(bs) {}

            void operator()(float x, float y) const {
                _bs.push_back(_grid.sample(x + .5f, y + .5f));
            }

            void operator()(int x, int y) const {
                operator()(float(x), float(y));
            }
        };

        struct candidate_qr_code {
            finder_pattern origin;
            finder_pattern right;
            finder_pattern bottom;

            std::optional<qr_format> format;
            std::optional<int> version;

            candidate_qr_code(finder_pattern origin, finder_pattern right, finder_pattern bottom)
                    : origin(std::move(origin)), right(std::move(right)), bottom(std::move(bottom)) {}

            [[nodiscard]] dynamic_bitset read_format_region_0(gray_u8 const& image) const {
                auto grid = qr_grid::from_finder(image, origin);
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
                auto grid = qr_grid::from_finder(image, right);
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
                    reader read(qr_grid::from_finder(image, right), r);

                    for (int i = 0; i < 8; i++) {
                        read(6 - i, 8);
                    }
                }


                {
                    reader read(qr_grid::from_finder(image, bottom), r);

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
                    reader read(qr_grid::from_finder(image, bottom), r);

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

            [[nodiscard]] homography make_homography() const {
                std::vector<std::pair<vector2d, vector2d>> pts;

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

                return estimate_homography(pts);
            }

            [[nodiscard]] qr_grid make_grid(gray_u8 const& image) const {
                auto homo = make_homography();
                return qr_grid(image, (bottom.gray_threshold + right.gray_threshold + origin.gray_threshold) / 3, homo);
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
    }



    std::vector<scanned_qr> scan_qr_codes(gray_u8 const& image,
                                          std::vector<finder_pattern> const& finder_patterns) {

        auto candidates = find_candidates(finder_patterns);

        std::vector<scanned_qr> res;

        for (auto& candidate : candidates) {
            if (!candidate.try_determine_format(image))
                continue;

            if (!candidate.try_determine_version(image))
                continue;

            // TODO: detect alignment patterns to add more features for homography

            auto grid = candidate.make_grid(image);

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

            if (err)
                continue;

            res.emplace_back(*candidate.version, *candidate.format, candidate.make_homography());
        }

        return res;
    }
}
