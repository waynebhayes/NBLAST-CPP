#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "core/Matrix.hpp"
#include "core/Neuron.hpp"
#include "utils/ArgParse.hpp"

#include <string>

typedef struct Score {
  std::string qid, tid;
  double score;
  void print(std::ostream &out, const std::string &sep) const;
} Score;

Score query(const Matrix &mat, const Neuron &query, const Neuron &target);

std::vector<Score> allByAll(const Matrix &mat,
                            const std::vector<Neuron> &neurons);

std::vector<Score> queryPairs(const Matrix &mat,
                              const std::vector<Neuron> &neurons,
                              const std::vector<std::pair<size_t, size_t>> &pairs);

// Best-scoring variants of the above (see -b). Results are sorted by score
// descending, ties broken by input order, so output is thread-count independent.
// allByAllBest considers each unordered pair once and skips self-pairs.
std::vector<Score> allByAllBest(const Matrix &mat,
                                const std::vector<Neuron> &neurons,
                                BestKind kind, double value);

std::vector<Score> queryPairsBest(const Matrix &mat,
                                  const std::vector<Neuron> &neurons,
                                  const std::vector<std::pair<size_t, size_t>> &pairs,
                                  BestKind kind, double value);

// Self-scores (each neuron against itself), computed in parallel.
std::vector<double> allSelfScores(const Matrix &mat,
                                  const std::vector<Neuron> &neurons);

struct Hit {
  size_t target;
  double score;
};

// For every query, its k best targets (score desc, ties by lower target index).
// Parallel over queries, so there is no cross-thread merging and the result is
// identical for any thread count. Memory is O(queries * k).
std::vector<std::vector<Hit>>
topKPerQuery(const Matrix &mat, const std::vector<Neuron> &queries,
             const std::vector<double> &querySelf,
             const std::vector<Neuron> &targets,
             const std::vector<double> &targetSelf, size_t k);

// Keeps only the best scores of an already-built vector (same ordering rules).
void keepBest(std::vector<Score> &scores, BestKind kind, double value);

std::vector<PAVector> calcDistsDotprods(const std::vector<Neuron> &queryNeurons,
                                         const std::vector<Neuron> &targetNeurons,
                                         const std::vector<std::pair<size_t, size_t>> &subset,
                                         bool ignoreSelf = true);

void updateMatrixWithResults(Matrix &mat, const std::vector<PAVector> &results);
using DoubleVector = std::vector<double>;
std::pair<DoubleVector, DoubleVector> generateBins(
    StringVector queryFilepathVector, StringVector targetFilepathVector,
    StringVector knownMatchesQueryVector, StringVector knownMatchesTargetVector,
    double binFactor, unsigned numIters, double coordScale = 1.0);

Matrix calcScoreMatrix(const Matrix &matchProb, const Matrix &randProb,
                       double logbase = 2.0, double epsilon = 1e-6);

#endif // PIPELINE_HPP
