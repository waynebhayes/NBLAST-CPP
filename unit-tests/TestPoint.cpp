#include "Test.hpp"
#include "core/Point.hpp"

#include <cmath>

TEST_CASE(test_angleMeasure_parallel) {
    Point a(1, 1.0, 0.0, 0.0, -1);
    Point b(2, 2.0, 0.0, 0.0, -1);
    REQUIRE_NEAR(a.angleMeasure(b, false), 1.0, 1e-9);
}

TEST_CASE(test_angleMeasure_antiparallel) {
    Point a(1,  1.0, 0.0, 0.0, -1);
    Point b(2, -1.0, 0.0, 0.0, -1);
    REQUIRE_NEAR(a.angleMeasure(b, false), 1.0, 1e-9);
}

TEST_CASE(test_angleMeasure_perpendicular) {
    Point a(1, 1.0, 0.0, 0.0, -1);
    Point b(2, 0.0, 1.0, 0.0, -1);
    REQUIRE_NEAR(a.angleMeasure(b, false), 0.0, 1e-9);
}

TEST_CASE(test_angleMeasure_zero_vector_returns_zero) {
    Point zero(1, 0.0, 0.0, 0.0, -1);
    Point a(2, 1.0, 0.0, 0.0, -1);
    REQUIRE_NEAR(zero.angleMeasure(a, false), 0.0, 1e-9);
    REQUIRE_NEAR(a.angleMeasure(zero, false), 0.0, 1e-9);
}

TEST_CASE(test_angleMeasure_sine_mode) {
    // 45-degree angle: cos=sqrt(2)/2, sin=sqrt(2)/2
    Point a(1, 1.0, 0.0, 0.0, -1);
    Point b(2, 1.0, 1.0, 0.0, -1);
    double expected_sine = std::sin(std::acos(1.0 / std::sqrt(2.0)));
    REQUIRE_NEAR(a.angleMeasure(b, true), expected_sine, 1e-9);
}

TEST_CASE(test_midpoint) {
    Point a(1, 0.0, 0.0, 0.0, -1);
    Point b(2, 4.0, 2.0, 6.0, -1);
    Point m = a.midpoint(b);
    REQUIRE_NEAR(m.x, 2.0, 1e-9);
    REQUIRE_NEAR(m.y, 1.0, 1e-9);
    REQUIRE_NEAR(m.z, 3.0, 1e-9);
}

TEST_CASE(test_dotProduct) {
    Point a(1, 3.0, 4.0, 0.0, -1);
    Point b(2, 1.0, 2.0, 5.0, -1);
    REQUIRE_NEAR(dotProduct(a, b), 11.0, 1e-9);  // 3*1 + 4*2 + 0*5 = 11
}

TEST_CASE(test_point_distance) {
    Point a(1, 0.0, 0.0, 0.0, -1);
    Point b(2, 3.0, 4.0, 0.0, -1);
    REQUIRE_NEAR(a.distance(b), 5.0, 1e-9);
}
