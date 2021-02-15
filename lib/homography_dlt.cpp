//
// Created by dcnick3 on 2/2/21.
//

#include <pqrs/homography_dlt.h>

#include <xtensor/xarray.hpp>
#include <xtensor/xtensor.hpp>


namespace pqrs {
    // time for linear algebra!

    namespace {
        typedef xt::xtensor<double, 2> matrix;
        typedef xt::xtensor<double, 1> vector;

        matrix product(matrix const& a, matrix const& b) {
            // just a conventional matrix product
            assert(a.shape(1) == b.shape(0));

            matrix r({a.shape(0), b.shape(1)}, 0);

            for (int i = 0; i < a.shape(0); i++)
                for (int j = 0; j < b.shape(1); j++)
                    for (int k = 0; k < a.shape(1); k++) {
                        r(i, j) += a(i, k) * b(k, j);
                    }

            return r;
        }

        vector product(matrix const& a, vector const& b) {
            // just a conventional matrix product
            assert(a.shape(1) == b.shape(0));

            vector r({a.shape(0)}, 0);

            for (int i = 0; i < a.shape(0); i++)
                for (int k = 0; k < a.shape(1); k++) {
                    r(i) += a(i, k) * b(k);
                }

            return r;
        }

        matrix inverse(matrix a) {
            // matrix inverse using Gauss-Jordan elimination
            assert(a.shape(0) == a.shape(1));
            auto N = a.shape(0);

            matrix r({N, N}, 0);
            for (int i = 0; i < N; i++)
                r(i, i) = 1.f;

            for (int i = 0; i < N; i++) {
                if (a(i, i) == 0.f)
                    throw std::domain_error("Encountered zero on main diagonal while trying to find an inverse");
                for (int j = 0; j < N; j++) {
                    if (i != j) {
                        auto ratio = a(j, i) / a(i, i);
                        for (int k = 0; k < N; k++) {
                            a(j, k) -= ratio * a(i, k);
                            r(j, k) -= ratio * r(i, k);
                        }
                    }
                }
            }
            for (int i = 0; i < N; i++) {
                auto val = a(i, i);
                for (int j = 0; j < N; j++) {
                    r(i, j) /= val;
                }
            }

            return r;
        }

        matrix pseudo_inverse(matrix const& a) {
            // assuming a has linearly independent columns
            // (what does it mean for LLS, again?)
            matrix transposed = xt::transpose(a);
            auto inv = inverse(product(transposed, a));
            return product(inv, transposed);
        }
    }

    homography estimate_homography(std::vector<std::pair<homo_vector2d, homo_vector2d>> const& points) {
        matrix A({2 * points.size(), 8}, 0.f);
        vector b({2 * points.size()}, 0.f);

        for (int i = 0; i < points.size(); i++) {
            auto[f, s] = points[i];

            A(2 * i, 3) = -s.w() * f.x();
            A(2 * i, 4) = -s.w() * f.y();
            A(2 * i, 5) = -s.w() * f.w();
            A(2 * i, 6) = s.y() * f.x();
            A(2 * i, 7) = s.y() * f.y();
            b(2 * i) = -s.y() * f.w();

            A(2 * i + 1, 0) = s.w() * f.x();
            A(2 * i + 1, 1) = s.w() * f.y();
            A(2 * i + 1, 2) = s.w() * f.w();
            A(2 * i + 1, 6) = -s.x() * f.x();
            A(2 * i + 1, 7) = -s.x() * f.y();
            b(2 * i + 1) = s.x() * f.w();
        }

        // now solve the LLS for mat * x = 0
        // x[0:7] would be the coefficients in homography matrix (h1-h8)
        // h9 is taken to be one (by reducing number of equations)

        // LLS using pseudo-inverse is calculated
        auto x = product(pseudo_inverse(A), b);
        homography_matrix res;
        res(0, 0) = x(0);
        res(0, 1) = x(1);
        res(0, 2) = x(2);
        res(1, 0) = x(3);
        res(1, 1) = x(4);
        res(1, 2) = x(5);
        res(2, 0) = x(6);
        res(2, 1) = x(7);
        res(2, 2) = 1;

        return homography(res);
    }

    homography estimate_homography(std::vector<std::pair<vector2d, vector2d>> const& points) {
        std::vector<std::pair<homo_vector2d, homo_vector2d>> conv;
        conv.reserve(points.size());

        for (auto const& p : points)
            conv.emplace_back(p);

        return estimate_homography(conv);
    }

    homography homography::inverse() const {
        return homography(pqrs::inverse(_matrix));
    }
}