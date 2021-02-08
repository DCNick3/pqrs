//
// Created by dcnick3 on 1/26/21.
//

#pragma once

#include <pqrs/point2d.h>

#include <vector>
#include <string>

#include <cassert>

namespace pqrs {
    // As for now, this is just a vector of vectors
    // Probably want to implement the data structure used in boofcv
    class contour_container {

    public:
        using inner = std::vector<point2d>;
        using _data_type = std::vector<inner>;
        using iterator = _data_type::iterator;

        inline void grow() {
            _data.emplace_back();
        }

        inline void add_point_to_tail(int x, int y) {
            add_point_to_tail({x, y});
        }

        inline void add_point_to_tail(point2d point_2_d) {
            assert(!_data.empty());
            _data.rbegin()->emplace_back(point_2_d);
        }

        [[nodiscard]] inline size_t size_of_tail() const {
            assert(!_data.empty());
            return _data.rbegin()->size();
        }

        inline void remove_tail() {
            assert(!_data.empty());
            _data.pop_back();
        }

        [[nodiscard]] std::string dump() const;

        inline iterator begin() { return _data.begin(); }
        inline iterator end() { return _data.end(); }

    private:
        std::vector<inner> _data;
    };
}