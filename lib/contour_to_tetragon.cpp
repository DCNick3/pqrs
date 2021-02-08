//
// Created by dcnick3 on 1/29/21.
//

#include <pqrs/contour_to_tetragon.h>
#include <pqrs/vector2d.h>
#include <pqrs/util.h>

#include <limits>
#include <cmath>
#include <list>
#include <stdexcept>
#include <algorithm>

// Dragons be here
// The original implementation is... Not so well-written; did not go through hassle of re-organizing it
// based on PolylineSplitMerge

namespace pqrs {
    namespace {
        constexpr double pi = 3.14159265358979323846;

        // maximum number of sides it will consider
        constexpr int max_sides = 4;

        // minimum number of sides that will be considered for the best polyline
        constexpr int min_sides = 4;

        // If the contour between two corners is longer than this multiple of the distance
        // between the two corners then it will be rejected as not convex
        constexpr double convex_test = 2.5;

        // maximum number of points along a side it will sample when computing a score
        // used to limit computational cost of large contours
        constexpr int max_number_of_side_samples = 50;

        // The minimum length of a side
        constexpr int minimum_side_length = 2;

        // If the score of a side is less than this it is considered a perfect fit and won't be split any more
        constexpr double threshold_side_split_score = 0.2;

        // When selecting the best model how much is a split penalized
        constexpr double corner_score_penalty = 0.4;

        struct candidate_polyline {
            std::vector<int> splits;
            double score{0};
            double max_side_score{0};
            std::vector<double> side_errors;
        };

        struct corner {
            int index;
            double side_error;

            // if this side was to be split this is where it would be split and what the scores
            // for the new sides would be
            int split_location{-1};
            double split_error_0{-1}, split_error_1{-1};

            // if a side can't be split (e.g. too small or already perfect)
            bool splittable{true};
        };

        struct impl {
            contour_container::inner const& contour;

            std::list<corner> corners;

            // List of all the found polylines and their score
            // indexed by (size - 3), stores best polygon of that size
            std::vector<candidate_polyline> polylines;

            using corner_it = decltype(corners)::iterator;

            using polyline_it = decltype(polylines)::iterator;

            explicit impl(const contour_container::inner &contour) : contour(contour) {}

            corner_it next_corner_circ(corner_it it) {
                auto next = std::next(it);
                if (next == corners.end()) return corners.begin();
                return next;
            }

            corner_it prev_corner_circ(corner_it it) {
                if (it == corners.begin())
                    it = corners.end();
                return std::prev(it);
            }

            /**
             * The seed corner is the point farther away from the first point. In a perfect polygon with no noise this should
             * be a corner.
             */
            int find_corner_seed() {
                auto a = contour[0];

                int best = -1;
                double best_distance = -std::numeric_limits<double>::max();

                for (int i = 1; i < contour.size(); i++) {
                    auto b = contour[i];

                    double d = (a - b).norm_squared();
                    if (d > best_distance) {
                        best_distance = d;
                        best = i;
                    }
                }

                return best;
            }

            /**
             * If point B is the point farthest away from A then by definition this means no other point can be farther
             * away.If the shape is convex then the line integration from A to B or B to A cannot be greater than 1/2
             * a circle. This test makes sure that the line integral meets the just described constraint and is thus
             * convex.
             *
             * @param index_a index of first point
             * @param index_b index of second point, which is the farthest away from A.
             * @return if it passes the sanity check
             */
            bool is_convex_using_max_distant_points(int index_a, int index_b) {
                auto d = (contour[index_a] - contour[index_b]).norm();

                // conservative upper bounds would be 1/2 a circle, including interior side.
                auto maxAllowed = (int)std::lround((pi + 1) * d);

                auto length0 = circular_distance_positive(index_a, index_b, contour.size());
                auto length1 = circular_distance_positive(index_b, index_a, contour.size());

                return length0 <= maxAllowed && length1 <= maxAllowed;
            }

            /**
             * Selects the best point to split a long along a contour. Start and end locations are always traversed in the
             * positive direction along the contour.
             *
             * @param index_a Start of line
             * @param index_b End of line
             * @return Where to split & score
             */
            std::pair<int, double> select_split_point(int index_a, int index_b) {
                std::pair<int, double> result{index_a, -1};

                // TODO: copypasta

                if (index_b >= index_a) {
                    for (int i = index_a + 1; i < index_b; i++) {
                        auto distance_sq = line_segment_to_point_distance_squared(
                                contour[index_a], contour[index_b], contour[i]
                        );

                        if (distance_sq > result.second) {
                            result = {i, distance_sq};
                        }
                    }
                } else {
                    auto distance = contour.size() - index_a + index_b;
                    for (int i = 1; i < distance; i++) {
                        auto index = (index_a + i) % contour.size();
                        auto distance_sq = line_segment_to_point_distance_squared(
                                contour[index_a], contour[index_b], contour[index]
                        );

                        if (distance_sq > result.second) {
                            result = {index, distance_sq};
                        }
                    }
                }

                return result;
            }

            void add_corner(int where) {
                corner c{};
                c.index = where;
                corners.emplace_back(c);
            }

            /**
             * Finds the point in the contour which maximizes the distance between points A
             * and B.
             *
             * @param index_a Index of point A
             * @param index_b Index of point B
             * @return Index of maximal distant point
             */
            int maximum_distance(int index_a, int index_b ) {
                auto a = contour[index_a];
                auto b = contour[index_b];

                int best = -1;
                double best_distance = -std::numeric_limits<double>::max();

                for (int i = 0; i < contour.size(); i++) {
                    auto c = contour[i];
                    // can't sum sq distance because some skinny shapes it maximizes one and not the other
                    //double d = Math.sqrt(distanceSq(a,c)) + Math.sqrt(distanceSq(b,c));
                    auto d = (a - c).manhattan_norm() + (b - c).manhattan_norm();
                    if (d > best_distance) {
                        best_distance = d;
                        best = i;
                    }
                }

                return best;
            }

            /**
             * Make sure the next corner after the head is the closest one to the head
             */
            void ensure_triangle_order() {
                auto it = corners.begin();
                auto a = *it;
                it++;
                auto b = *it;
                it++;
                auto c = *it;


                auto distB = circular_distance_positive(a.index, b.index, contour.size());
                auto distC = circular_distance_positive(a.index, c.index, contour.size());

                if (distB > distC) {
                    corners.clear();
                    corners.push_back(a);
                    corners.push_back(c);
                    corners.push_back(b);
                }
            }

            /**
             * Checks to see if the side could belong to a convex shape
             */
            bool is_side_convex(corner_it e1) {
                // a conservative estimate for concavity. Assumes a triangle and that the farthest
                // point is equal to the distance between the two corners

                auto e2 = next_corner_circ(e1);

                int length = circular_distance_positive(e1->index, e2->index, contour.size());

                auto p0 = contour[e1->index];
                auto p1 = contour[e2->index];

                double d = (p0 - p1).norm();

                if (length >= d * convex_test) {
                    return false;
                }
                return true;
            }

            /**
             * Scores a side based on the sum of Euclidean distance squared of each point along the line. Euclidean squared
             * is used because its fast to compute
             *
             * @param index_a first index. Inclusive
             * @param index_b last index. Exclusive
             */
            double compute_side_error(int index_a, int index_b) {

                // TODO: copypasta

                // don't sample the end points because the error will be zero by definition
                int num_samples;
                double sum_of_distances = 0;
                int length;
                if (index_b >= index_a) {
                    length = index_b - index_a - 1;
                    num_samples = std::min(length, max_number_of_side_samples);
                    for (int i = 0; i < num_samples; i++) {
                        auto index = index_a + 1 + length * i / num_samples;

                        sum_of_distances += line_segment_to_point_distance_squared(
                                contour[index_a], contour[index_b],
                                contour[index]);
                    }
                    sum_of_distances /= num_samples;
                } else {
                    length = (int) contour.size() - index_a - 1 + index_b;
                    num_samples = std::min(length, max_number_of_side_samples);
                    for (int i = 0; i < num_samples; i++) {
                        auto where = length * i / num_samples;
                        auto index = (index_a + 1 + where) % contour.size();

                        sum_of_distances += line_segment_to_point_distance_squared(
                                contour[index_a], contour[index_b],
                                contour[index]);
                    }
                    sum_of_distances /= num_samples;
                }

                // handle divide by zero error
                if (num_samples > 0)
                    return sum_of_distances;
                else
                    return 0;
            }

            /**
             * Determines if the side can be split again. A side can always be split as long as
             * it's &ge; the minimum length or that the side score is larger the the split threshold
             *
             * @param e0 The side which is to be tested to see if it can be split
             * @param mustSplit if true this will force it to split even if the error would prevent it from splitting
             * @return true if it can be split or false if not
             */
            bool can_be_split(corner_it e0, bool mustSplit) {
                auto e1 = next_corner_circ(e0);

                // NOTE: The contour is passed in but only the size of the contour matters. This was done to prevent
                //       changing the signature if the algorithm was changed later on.
                int length = circular_distance_positive(e0->index, e1->index, contour.size());

                // needs to be <= to prevent it from trying to split a side less than 1
                // times two because the two new sides would have to have a length of at least min
                if (length <= 2 * minimum_side_length) {
                    return false;
                }

                // threshold is greater than zero ti prevent it from saying it can split a perfect side
                return mustSplit || e0->side_error > threshold_side_split_score;
            }

            /**
             * Selects and splits the side defined by the e0 corner. If convex a check is performed to
             * ensure that the polyline will be convex still.
             */
            void set_split_variables(corner_it e0, corner_it e1) {

//		int distance0 = CircularIndex.distanceP(e0.object.index, e1.object.index, contour.size());

                int index0 = circular_mod(e0->index + minimum_side_length, contour.size());
                int index1 = circular_mod(e1->index - minimum_side_length, contour.size());

                auto results = select_split_point(index0, index1);

                // perform the split if it would result in a convex polygon
                auto a = contour[e0->index];
                auto b = contour[results.first];
                auto c = contour[next_corner_circ(e0)->index];

                if (polygon_is_positive_z(a, b, c)) {
                    e0->splittable = false;
                    return;
                }

                // see if this would result in a side that's too small
                int dist0 = circular_distance_positive(e0->index, results.first, contour.size());
                if (dist0 < minimum_side_length || (contour.size() - dist0) < minimum_side_length) {
                    std::terminate(); // Should be impossible
                }

                // this function is only called if splitable is set to true so no need to set it again
                e0->split_location = results.first;
                e0->split_error_0 = compute_side_error(e0->index, results.first);
                e0->split_error_1 = compute_side_error(results.first, e1->index);

                if (e0->split_location >= contour.size())
                    std::terminate(); // Egads
            }

            /**
             * Computes the split location and the score of the two new sides if it's split there
             */
            void compute_potential_split_score(corner_it e0, bool mustSplit) {
                auto e1 = next_corner_circ(e0);

                e0->splittable = can_be_split(e0, mustSplit);

                if (e0->splittable) {
                    set_split_variables(e0, e1);
                }
            }
            /**
             * Computes the score and potential split for each side
             * @param contour
             * @return
             */
            bool initialize_score() {
                // Score each side
                auto e = corners.begin();
                auto end = corners.end();
                while (e != end) {
                    if (!is_side_convex(e))
                        return false;

                    auto n = std::next(e);

                    double error;
                    if (n == end) {
                        error = compute_side_error(e->index, corners.begin()->index);
                    } else {
                        error = compute_side_error(e->index, n->index);
                    }
                    e->side_error = error;
                    e++;
                }

                // Compute what would happen if a side was split
                e = corners.begin();
                while (e != end) {
                    compute_potential_split_score(e, corners.size() < min_sides);
                    e++;
                }

                return true;
            }

            /**
             * Select an initial triangle. A good initial triangle is needed. By good it
             * should minimize the error of the contour from each side
             */
            bool find_initial_triangle() {
                // find the first estimate for a corner
                int corner_seed = find_corner_seed();

                // see if it can reject the contour immediately
                if (!is_convex_using_max_distant_points(0, corner_seed))
                    return false;

                // Select the second corner.
                auto results_a = select_split_point(0, corner_seed);
                auto results_b = select_split_point(corner_seed, 0);

                if (results_a.second > results_b.second) {
                    // score a is better
                    add_corner(results_a.first);
                    add_corner(corner_seed);
                } else {
                    add_corner(corner_seed);
                    add_corner(results_b.first);
                }

                // Select the third corner. Initial triangle will be complete now
                // the third corner will be the one which maximizes the distance from the first two
                int index0 = corners.begin()->index;
                int index1 = std::next(corners.begin())->index;
                int index2 = maximum_distance(index0, index1);
                add_corner(index2);

                // enforce CCW requirement
                ensure_triangle_order();

                return initialize_score();
            }

            /**
             * Computes the score for a list
             */
            double compute_score() {
                double sum_sides = 0;

                for (auto const& c : corners) {
                    sum_sides += c.side_error;
                }

                int num_sides = corners.size();

                return sum_sides / num_sides + corner_score_penalty * num_sides;
            }

            /**
             * Saves the current polyline
             *
             * @return true if the polyline is better than any previously saved result false if not and it wasn't saved
             */
            bool save_polyline() {
                int N = 3;

                // if a polyline of this size has already been saved then over write it
                polyline_it c;
                if (corners.size() <= polylines.size() + N - 1) {
                    c = polylines.begin() + (corners.size() - N);
                    // sanity check
                    if (c->splits.size() != corners.size())
                        std::terminate(); // Egads saved polylines aren't in the expected order
                } else {
                    polylines.emplace_back();
                    c = polylines.end() - 1;
                    c->score = std::numeric_limits<double>::max();
                }

                auto found_score = compute_score();

                // only save the results if it's an improvement
                if (c->score > found_score) {
                    c->score = found_score;
                    c->splits.clear();
                    c->side_errors.clear();

                    double max_side_error = 0;

                    for (auto const& e : corners) {
                        max_side_error = std::max(max_side_error, e.side_error);
                        c->splits.push_back(e.index);
                        c->side_errors.push_back(e.side_error);
                    }

                    c->max_side_score = max_side_error;
                    return true;
                } else {
                    return false;
                }
            }

            /**
             * Selects the best side to split the polyline at.
             * @return the selected side or corners.end() if the score will not be improved if any of the sides are split
             */
            corner_it select_corner_to_split() {
                auto selected = corners.end();
                double best_change = 0;

                // Pick the side that if split would improve the overall score the most
                auto e = corners.begin();
                auto end = corners.end();

                while (e != end) {
                    corner &c = *e;
                    if (!c.splittable) {
                        e++;
                        continue;
                    }

                    // compute how much better the score will improve because of the split
                    double change = c.side_error * 2 - c.split_error_0 - c.split_error_1;
                    // it was found that selecting for the biggest change tends to produce better results
                    if (change < 0) {
                        change = -change;
                    }
                    if (change > best_change) {
                        best_change = change;
                        selected = e;
                    }
                    e++;
                }

                return selected;
            }

            /**
             * Increase the number of sides in the polyline. This is done greedily selecting the side which would improve the
             * score by the most of it was split.
             * @return true if a split was selected and false if not
             */
            bool increase_number_of_sides_by_one() {
                auto selected = select_corner_to_split();

                // No side can be split
                if (selected == corners.end())
                    return false;

                // Update the corner who's side was just split
                selected->side_error = selected->split_error_0;
                // split the selected side and add a new corner
                corner c{};
                c.index = selected->split_location;
                c.side_error = selected->split_error_1;
                auto corner_e = corners.insert(std::next(selected), c);

                // see if the new side could be convex
                if (!is_side_convex(selected))
                    return false;
                else {
                    // compute the score for sides which just changed
                    compute_potential_split_score(corner_e, corners.size() < min_sides);
                    compute_potential_split_score(selected, corners.size() < min_sides);

                    // Save the results
                    save_polyline();

                    return true;
                }
            }

            /**
             * Selects the best corner to remove. If no corner was found that can be removed then null is returned
             * @return The corner to remove. Should only return corners.end() if there are 3 sides or less
             */
            corner_it select_corner_to_remove(double& side_error) {
                if (corners.size() <= 3)
                    return corners.end();

                // Pick the side that if split would improve the overall score the most
                auto target = corners.begin(), end = corners.end();

                auto best = corners.end();
                double best_score = -std::numeric_limits<double>::max();

                while (target != end) {
                    auto p = prev_corner_circ(target);
                    auto n = next_corner_circ(target);

                    // just contributions of the corners in question
                    double before = (p->side_error + target->side_error) / 2.0 + corner_score_penalty;
                    double after = compute_side_error(p->index, n->index);

                    if (before - after > best_score) {
                        best_score = before - after;
                        best = target;
                        side_error = after;
                    }
                    target++;
                }

                return best;
            }

            /**
             * Remove the corner from the current polyline. If the new polyline has a better score than the currently
             * saved one with the same number of corners save it
             * @param corner The corner to removed
             */
            bool remove_corner_and_save_polyline(corner_it corner, double side_error_after_removed) {
                auto p = prev_corner_circ(corner);

                // go through the hassle of passing in this value instead of recomputing it
                // since recomputing it isn't trivial
                p->side_error = side_error_after_removed;
                corners.erase(corner);
                // the line below is commented out because right now the current algorithm will
                // never grow after removing a corner. If this changes in the future uncomment it
//			computePotentialSplitScore(contour,p);
                return save_polyline();
            }

            void sequential_side_fit() {
                // by finding more corners than necessary it can recover from mistakes previously
                int limit = 2 * max_sides;
                if (limit <= 0)
                    limit = contour.size(); // handle the situation where it overflows
                while (corners.size() < limit) {
                    if (!increase_number_of_sides_by_one()) {
                        break;
                    }
                }
                // remove corners and recompute scores. If the result is better it will be saved
                while (true) {
                    double sideError;

                    auto c = select_corner_to_remove(sideError);
                    if (c != corners.end()) {
                        remove_corner_and_save_polyline(c, sideError);
                    } else {
                        break;
                    }
                }
            }

            std::optional<candidate_polyline> process() {
                if (contour.size() < 3)
                    return {};

                if (!find_initial_triangle())
                    return {};

                save_polyline();

                sequential_side_fit();

                int MIN_SIZE = 3;

                double best_score = std::numeric_limits<double>::max();
                std::optional<candidate_polyline> best_polyline;
                int best_size = -1;

                for (int i = 0; i < std::min(max_sides - (MIN_SIZE - 1), (int) polylines.size()); i++) {
                    if (polylines[i].score < best_score) {
                        best_polyline = polylines[i];
                        best_score = best_polyline->score;
                        best_size = i + MIN_SIZE;
                    }
                }

                // There was no good match within the min/max size requirement
                if (best_size < min_sides) {
                    return {};
                }

                // make sure all the sides are within error tolerance
                for (int i = 0, j = best_size - 1; i < best_size; j = i, i++) {
                    auto a = contour[best_polyline->splits[i]];
                    auto b = contour[best_polyline->splits[j]];

                    auto length = (a - b).norm();
                    auto threshold_side_error = std::max(3., length * .12);
                    if (best_polyline->side_errors[i] >= threshold_side_error * threshold_side_error) {
                        return {};
                    }
                }

                return best_polyline;
            }
        };

        // corresponds to RefinePolyLineCorner
        struct refine {
            contour_container::inner const &contour;
            std::vector<int> &corners;

            // maximum number of iterations
            static constexpr int max_iterations = 10;

            // maximum number of samples along the line
            static constexpr int max_line_samples = 20;

            // the radius it will search around. computed when contour is passed in
            int search_radius;

            refine(const contour_container::inner &contour,
                   std::vector<int> &corners)
                    : contour(contour),
                      corners(corners) {

                search_radius = std::min(6, std::max((int)contour.size() / 12, 3));
            }

            inline double distance(int c0, int c1, point2d p) {
                return line_to_point_distance_squared(contour[c0], contour[c1], p);
            }

            /**
             * Sum of Euclidean distance of contour points along the line
             */
            double distance_sum(int c0, int c1) {
                double total = 0.;
                if (c0 < c1) {
                    int length = c1 - c0 + 1;
                    int samples = std::min(max_line_samples, length);

                    for (int i = 0; i < samples; i++) {
                        int index = c0 + i * (length - 1) / (samples - 1);
                        total += distance(c0, c1, contour[index]);
                    }
                } else {
                    int length_first = (int)contour.size() - c0;
                    int length_second = c1 + 1;

                    int length = length_first + c1 + 1;
                    int samples = std::min(max_line_samples, length);

                    int samples_first = samples * length_first / length;
                    int samples_second = samples * length_second / length;

                    for (int i = 0; i < samples_first; i++) {
                        int index = c0 + i * length_first / (samples - 1);
                        total += distance(c0, c1, contour[index]);
                    }
                    for (int i = 0; i < samples_second; i++) {
                        int index = i * length_second / (samples - 1);
                        total += distance(c0, c1, contour[index]);
                    }
                }
                return total;
            }

            /**
             * Computes the distance between the two lines defined by corner points in the contour
             *
             * @param contour list of contour points
             * @param c0 end point of line 0
             * @param c1 start of line 0 and 1
             * @param c2 end point of line 1
             * @param offset added to c1 to make start of lines
             * @return sum of distance of points along contour
             */
            double compute_cost(int c0, int c1, int c2, int offset) {
                c1 = circular_mod(c1 + offset, contour.size());

                return distance_sum(c0, c1) + distance_sum(c1, c2);
            }

            /**
             * Searches around the current c1 point for the best place to put the corner
             *
             * @return location of best corner in local search region
             */
            int optimize_one(int c0, int c1, int c2) {
                double best_cost = compute_cost(c0, c1, c2, 0);
                int best_offset = 0;
                for (int offset = -search_radius; offset <= search_radius; offset++) {
                    if (offset == 0) {
                        // if it found a better point in the first half stop the search since that's probably the correct
                        // direction.  Could be improved by remember past search direction
                        if (best_offset != 0)
                            break;
                    } else {
                        double cost = compute_cost(c0, c1, c2, offset);
                        if (cost < best_cost) {
                            best_cost = cost;
                            best_offset = offset;
                        }
                    }
                }
                return circular_mod(c1 + best_offset, contour.size());
            }

            bool process() {
                if (corners.size() < 3)
                    return false;

                bool change = true;
                for (int iteration = 0; iteration < max_iterations && change; iteration++) {
                    change = false;

                    for (int i = 0; i < corners.size(); i++) {
                        int i0 = circular_mod(i - 1, corners.size());
                        int i2 = circular_mod(i + 1, corners.size());

                        int improved = optimize_one(corners[i0], corners[i], corners[i2]);
                        if (improved != corners[i]) {
                            corners[i] = improved;
                            change = true;
                        }
                    }
                }

                return true;
            }
        };

        // corresponds to AdjustPolygonForThresholdBias
        void remove_bias(std::vector<vector2d>& polygon) {
            std::vector<std::pair<vector2d, vector2d>> segments;

            int N = polygon.size();
            segments.resize(N);

            // Apply the adjustment independently to each side
            for (int i = N - 1, j = 0; j < N; i = j, j++) {

                int ii = j, jj = i;

                auto a = polygon[ii], b = polygon[jj];

                float dx = b.x() - a.x();
                float dy = b.y() - a.y();
                float l = std::sqrt(dx*dx + dy*dy);
                if (l == 0)
                    std::terminate(); // Two identical corners!

                // only needs to be shifted in two directions
                if (dx < 0)
                    dx = 0;
                if (dy > 0)
                    dy = 0;

                auto adj = vector2d(-dy / l, dx / l);
                segments[ii] = {a + adj, b + adj};
            }

            // Find the intersection between the adjusted lines to convert it back into polygon format
            for (int i = N - 1, j = 0; j < N; i = j, j++) {
                int ii, jj;
                    ii = j;
                    jj = i;

                auto intersection = lines_intersection(segments[ii].first, segments[ii].second,
                                                       segments[jj].first, segments[jj].second);

                if (intersection) {
                    // very acute angles can cause a large delta. This is conservative and prevents that
                    if ((*intersection - polygon[jj]).norm_squared() < 20) {
                        polygon[jj] = *intersection;
                    }
                }
            }

            // we do not try to remove adjacent duplicated, because it rarely happens with tetragons
        }

        // renamed from UtilPolygons2D_I32.isCCW because of coordinate systems
        // see https://github.com/lessthanoptimal/BoofCV/issues/210
        bool is_polygon_cw(std::vector<vector2d> const& pp) {
            int N = pp.size();
            int sign = 0;
            for (int i = 0; i < N; i++) {
                int j = (i + 1) % N;
                int k = (i + 2) % N;

                if (polygon_is_positive_z(pp[i], pp[j], pp[k]))
                    sign++;
                else
                    sign--;
            }

            return sign < 0;
        }
    }

    std::optional<tetragon> contour_to_tetragon(contour_container::inner const& contour) {
        impl impl_class{contour};

        auto best_fit = impl_class.process();

        if (!best_fit)
            return {};

        assert(best_fit->splits.size() == 4);

        refine refine_class{contour, best_fit->splits};

        if (!refine_class.process())
            return {};

        std::vector<vector2d> ret;

        for (auto i : best_fit->splits) {
            ret.emplace_back(contour[i]);
        }

        remove_bias(ret);

        if (!is_polygon_cw(ret))
            std::reverse(ret.begin(), ret.end());

        return {{ret[0], ret[1], ret[2], ret[3]}};
    }
}
