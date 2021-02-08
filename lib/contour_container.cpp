//
// Created by dcnick3 on 1/28/21.
//

#include <pqrs/contour_container.h>

#include <sstream>

namespace pqrs {
    std::string contour_container::dump() const {
        std::stringstream ss;

        for (auto const& cont : _data) {
            for (int i = 0; i < cont.size(); i++) {

                ss << "{ " << cont[i].x() << ", " << cont[i].y() << " }";

                if (i != cont.size() - 1) {
                    ss << ", ";
                }
            }
            ss << "\n";
        }

        return ss.str();
    }
}
