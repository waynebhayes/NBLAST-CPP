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

std::vector<PAVector> calcDistsDotprods(const std::vector<Neuron> &queryNeurons,
                                         const std::vector<Neuron> &targetNeurons,
                                         const std::vector<std::pair<size_t, size_t>> &subset,
                                         bool ignoreSelf = true);

void updateMatrixWithResults(Matrix &mat, const std::vector<PAVector> &results);
using DoubleVector = std::vector<double>;
std::pair<DoubleVector, DoubleVector> generateBins(
    StringVector queryFilepathVector, StringVector targetFilepathVector,
    StringVector knownMatchesQueryVector, StringVector knownMatchesTargetVector,
    double binFactor, unsigned numIters);

Matrix calcScoreMatrix(const Matrix &matchProb, const Matrix &randProb,
                       double logbase = 2.0, double epsilon = 1e-6);

#endif // PIPELINE_HPP
