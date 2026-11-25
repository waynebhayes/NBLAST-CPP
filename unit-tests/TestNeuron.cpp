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

// ---------------- alpha (NBLAST v2) ----------------

// A matrix that only rewards well-aligned matches: angle measure <= 0.5 scores 0,
// > 0.5 scores 1, so alpha weighting (which shrinks the angle measure) shows up.
static Matrix makeAngleStepMatrix() {
    Matrix mat;
    mat.getDistanceBins() = {1e9};
    mat.getAngleBins()    = {0.5, 1.0};
    mat.getTable()        = {{0.0, 1.0}};
    mat.setInterpolate(false);
    return mat;
}

TEST_CASE(test_alpha_off_by_default) {
    Neuron n("unit-tests/test_data/swc/alpha/line.swc");
    REQUIRE(!n.hasAlpha());
}

TEST_CASE(test_alpha_line_is_one) {
    Neuron n("unit-tests/test_data/swc/alpha/line.swc", 1.0, 5);
    REQUIRE(n.hasAlpha());
    REQUIRE_EQ(n.getAlpha().size(), static_cast<size_t>(24));
    for (double a : n.getAlpha())
        REQUIRE_NEAR(a, 1.0, 1e-9);
}

TEST_CASE(test_alpha_isotropic_is_zero) {
    Neuron n("unit-tests/test_data/swc/alpha/lattice.swc", 1.0, 27);
    // segment 14 of 27 is the lattice centre; every point is in its neighbourhood
    REQUIRE_NEAR(n.getAlpha()[13], 0.0, 1e-9);
    for (double a : n.getAlpha()) {
        REQUIRE(a >= 0.0);
        REQUIRE(a <= 1.0);
    }
}

TEST_CASE(test_alpha_k_larger_than_neuron) {
    // k is clamped to the number of segments rather than failing
    Neuron n("unit-tests/test_data/swc/alpha/line.swc", 1.0, 1000);
    REQUIRE_EQ(n.getAlpha().size(), static_cast<size_t>(24));
    REQUIRE_NEAR(n.getAlpha()[0], 1.0, 1e-9);
}

TEST_CASE(test_alpha_self_score_equals_one) {
    Matrix mat = makeAngleStepMatrix();
    Neuron n("unit-tests/test_data/swc/alpha/line.swc", 1.0, 5);
    REQUIRE_NEAR(n.score(n, mat), 1.0, 1e-9);
}

TEST_CASE(test_alpha_one_leaves_scores_unchanged) {
    Matrix mat = makeAngleStepMatrix();
    Neuron plain("unit-tests/test_data/swc/alpha/line.swc");
    Neuron weighted("unit-tests/test_data/swc/alpha/line.swc", 1.0, 5);
    REQUIRE_NEAR(weighted.directedScore(weighted, mat),
                 plain.directedScore(plain, mat), 1e-9);
}

TEST_CASE(test_alpha_downweights_isotropic_matches) {
    Matrix mat = makeAngleStepMatrix();
    Neuron plain("unit-tests/test_data/swc/alpha/lattice.swc");
    Neuron weighted("unit-tests/test_data/swc/alpha/lattice.swc", 1.0, 27);
    // without alpha every segment matches itself perfectly (27 x 1); with alpha
    // the centre (alpha 0) and other low-alpha segments drop below the step
    REQUIRE_NEAR(plain.directedScore(plain, mat), 27.0, 1e-9);
    REQUIRE(weighted.directedScore(weighted, mat) < 27.0);
}

TEST_CASE(test_alpha_needs_both_neurons) {
    // weighting applies only when both neurons carry alpha
    Matrix mat = makeAngleStepMatrix();
    Neuron plain("unit-tests/test_data/swc/alpha/lattice.swc");
    Neuron weighted("unit-tests/test_data/swc/alpha/lattice.swc", 1.0, 27);
    REQUIRE_NEAR(plain.directedScore(weighted, mat),
                 plain.directedScore(plain, mat), 1e-9);
}
