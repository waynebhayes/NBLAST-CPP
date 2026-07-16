#include "Test.hpp"
#include "analysis/Pipeline.hpp"
#include "core/Matrix.hpp"
#include "core/Point.hpp"

static Matrix makeBinMatrix() {
    Matrix mat;
    mat.getDistanceBins() = {100.0};
    mat.getAngleBins()    = {1.0};
    mat.getTable()        = {{0.0}};
    mat.setInterpolate(false);
    return mat;
}

// updateMatrixWithResults

TEST_CASE(test_updateMatrix_skips_invalid_query_id) {
    Matrix mat = makeBinMatrix();
    PAVector v = {PointAlignment(-1, 5, 1.0, 0.5)};
    updateMatrixWithResults(mat, {v});
    REQUIRE_EQ(mat.getTable()[0][0], 0.0);
}

TEST_CASE(test_updateMatrix_skips_invalid_target_id) {
    Matrix mat = makeBinMatrix();
    PAVector v = {PointAlignment(5, -1, 1.0, 0.5)};
    updateMatrixWithResults(mat, {v});
    REQUIRE_EQ(mat.getTable()[0][0], 0.0);
}

TEST_CASE(test_updateMatrix_skips_both_invalid) {
    Matrix mat = makeBinMatrix();
    PAVector v = {PointAlignment(-1, -1, 1.0, 0.5)};
    updateMatrixWithResults(mat, {v});
    REQUIRE_EQ(mat.getTable()[0][0], 0.0);
}

TEST_CASE(test_updateMatrix_counts_valid_entry) {
    Matrix mat = makeBinMatrix();
    PAVector v = {PointAlignment(1, 5, 1.0, 0.5)};
    updateMatrixWithResults(mat, {v});
    REQUIRE_EQ(mat.getTable()[0][0], 1.0);
}

TEST_CASE(test_updateMatrix_counts_multiple_valid) {
    Matrix mat = makeBinMatrix();
    PAVector v = {
        PointAlignment(1, 2, 1.0, 0.5),
        PointAlignment(3, 4, 1.0, 0.5),
    };
    updateMatrixWithResults(mat, {v});
    REQUIRE_EQ(mat.getTable()[0][0], 2.0);
}

// calcScoreMatrix

TEST_CASE(test_calcScoreMatrix_logodds) {
    // log2((match+eps)/(rand+eps)) with eps=1e-6
    Matrix matchProb;
    matchProb.getDistanceBins() = {1.0};
    matchProb.getAngleBins()    = {1.0};
    matchProb.getTable()        = {{0.5}};

    Matrix randProb;
    randProb.getDistanceBins() = {1.0};
    randProb.getAngleBins()    = {1.0};
    randProb.getTable()        = {{0.25}};

    Matrix score = calcScoreMatrix(matchProb, randProb, 2.0, 1e-6);
    double expected = std::log((0.5 + 1e-6) / (0.25 + 1e-6)) / std::log(2.0);
    REQUIRE_NEAR(score.getTable()[0][0], expected, 1e-9);
}

TEST_CASE(test_calcScoreMatrix_equal_probs_near_zero) {
    // equal match and rand → score ≈ 0
    Matrix prob;
    prob.getDistanceBins() = {1.0};
    prob.getAngleBins()    = {1.0};
    prob.getTable()        = {{0.1}};

    Matrix score = calcScoreMatrix(prob, prob, 2.0, 1e-6);
    REQUIRE_NEAR(score.getTable()[0][0], 0.0, 1e-9);
}
