#include "Test.hpp"
#include "analysis/Pipeline.hpp"
#include "core/Matrix.hpp"
#include "core/Neuron.hpp"

// A flat-score matrix: every (distance, angle) pair returns the same value.
// Useful for self-score tests since all midpoints match themselves at distance=0,
// so the raw self-score equals the midpoint count regardless of angle bin.
static Matrix makeFlatMatrix(double score = 1.0) {
    Matrix mat;
    mat.getDistanceBins() = {1e9};
    mat.getAngleBins()    = {1.0};
    mat.getTable()        = {{score}};
    mat.setInterpolate(false);
    return mat;
}

TEST_CASE(test_selfScore_positive) {
    Matrix mat = makeFlatMatrix();
    Neuron n("unit-tests/test_data/swc/banc/banc-0.swc");
    REQUIRE(n.selfScore(mat) > 0.0);
}

TEST_CASE(test_score_self_equals_one) {
    Matrix mat = makeFlatMatrix();
    Neuron n("unit-tests/test_data/swc/banc/banc-0.swc");
    // score(A, A) = (selfA/selfA + selfA/selfA) / 2 = 1.0 for any non-zero self
    REQUIRE_NEAR(n.score(n, mat), 1.0, 1e-9);
}

TEST_CASE(test_score_with_precomputed_self_matches_default) {
    Matrix mat = makeFlatMatrix();
    Neuron q("unit-tests/test_data/swc/banc/banc-0.swc");
    Neuron t("unit-tests/test_data/swc/banc/banc-1.swc");

    double qs = q.selfScore(mat);
    double ts = t.selfScore(mat);

    double s1 = q.score(t, mat);
    double s2 = q.score(t, mat, qs, ts);
    REQUIRE_NEAR(s1, s2, 1e-9);
}

TEST_CASE(test_allByAll_returns_n_squared) {
    Matrix mat = makeFlatMatrix();
    std::vector<Neuron> neurons;
    neurons.emplace_back("unit-tests/test_data/swc/banc/banc-0.swc");
    neurons.emplace_back("unit-tests/test_data/swc/banc/banc-1.swc");

    auto scores = allByAll(mat, neurons);
    REQUIRE_EQ(scores.size(), static_cast<size_t>(4));
}

TEST_CASE(test_allByAll_diagonal_is_one) {
    Matrix mat = makeFlatMatrix();
    std::vector<Neuron> neurons;
    neurons.emplace_back("unit-tests/test_data/swc/banc/banc-0.swc");
    neurons.emplace_back("unit-tests/test_data/swc/banc/banc-1.swc");

    auto scores = allByAll(mat, neurons);
    // Row-major: [0,0], [0,1], [1,0], [1,1]
    REQUIRE_NEAR(scores[0].score, 1.0, 1e-9);  // (0,0) self
    REQUIRE_NEAR(scores[3].score, 1.0, 1e-9);  // (1,1) self
}

TEST_CASE(test_allByAll_symmetric) {
    Matrix mat = makeFlatMatrix();
    std::vector<Neuron> neurons;
    neurons.emplace_back("unit-tests/test_data/swc/banc/banc-0.swc");
    neurons.emplace_back("unit-tests/test_data/swc/banc/banc-1.swc");

    auto scores = allByAll(mat, neurons);
    // score(A,B) == score(B,A) because the mean normalization is symmetric
    REQUIRE_NEAR(scores[1].score, scores[2].score, 1e-9);
}
