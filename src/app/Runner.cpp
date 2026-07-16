#include "Runner.hpp"
#include "analysis/Pipeline.hpp"
#include "core/Matrix.hpp"
#include "io/FileIO.hpp"
#include "utils/ArgParse.hpp"
#include "utils/Logging.hpp"
#include "utils/StringUtils.hpp"
#include "utils/Timer.hpp"

#include <iostream>

void runQueryMode(const Args &a) {
  LOG_INFO("Using Scoring Matrix: \"%s\"", a.matrixFilepath.c_str());

  Matrix mat = loadMatrixFromTSV(a.matrixFilepath);
  mat.setInterpolate(a.doInterpolation);
  TimerStats ts;
  if (a.positionalArgs.empty()) {
    if (a.doAllByAll) {
      std::vector<std::string> neuronFilepaths;
      std::string neuronID;
      while (std::cin >> neuronID) {
        std::string neuronFilepath =
            filenameToPath(a.queryDatasetFilepath, neuronID, ".swc");
        neuronFilepaths.push_back(neuronFilepath);
      }
      std::vector<Neuron> neurons = loadNeurons(neuronFilepaths);
      std::vector<Score> scoreVector = allByAll(mat, neurons);
      for (const Score &s : scoreVector) {
        s.print(std::cout, "\t");
      }
      std::cout.flush();
      return;
    } else {
      std::vector<std::string> queryFilepaths;
      std::vector<std::string> targetFilepaths;
      std::string queryID, targetID;
      while (std::cin >> queryID >> targetID) {
        std::string queryFilepath =
            filenameToPath(a.queryDatasetFilepath, queryID, ".swc");
        queryFilepaths.push_back(queryFilepath);
        std::string targetFilepath =
            filenameToPath(a.targetDatasetFilepath, targetID, ".swc");
        targetFilepaths.push_back(targetFilepath);
      }
      std::vector<Neuron> queryNeurons = loadNeurons(queryFilepaths);
      std::vector<Neuron> targetNeurons = loadNeurons(targetFilepaths);
      if (queryNeurons.size() != targetNeurons.size()) {
        throw std::runtime_error(
            "query neuron list doesnt match target neuron list");
      }
      std::vector<Score> scoreVector;
      scoreVector.reserve(queryNeurons.size());
      for (size_t i = 0; i < queryNeurons.size(); ++i) {
        scoreVector.push_back(timeFunction(ts, [&]() {
          return query(mat, queryNeurons[i], targetNeurons[i]);
        }));
      }

      for (const Score &s : scoreVector) {
        s.print(std::cout, "\t");
      }
      std::cout.flush();
      if (a.doDump) {
        std::ofstream tout("out/query-times.txt");
        ts.print(tout);
        tout.close();
      }
      return;
    }
  }
  std::string queryFilepath =
      filenameToPath(a.queryDatasetFilepath, a.positionalArgs[0], ".swc");
  Neuron queryNeuron = Neuron(queryFilepath);
  std::vector<std::string> targetFilepaths;
  for (size_t i = 1; i < a.positionalArgs.size(); i++) {
    std::string targetFilepath =
        filenameToPath(a.targetDatasetFilepath, a.positionalArgs[i], ".swc");
    targetFilepaths.push_back(targetFilepath);
  }
  std::vector<Neuron> targetNeurons = loadNeurons(targetFilepaths);
  std::vector<Score> scoreVector;
  scoreVector.reserve(targetNeurons.size());
  for (size_t j = 0; j < targetNeurons.size(); j++) {
    scoreVector.push_back(query(mat, queryNeuron, targetNeurons[j]));
  }
  for (const Score &s : scoreVector) {
    s.print(std::cout, "\t");
  }
  std::cout.flush();
}

void runGeneratorMode(const Args &a) {
  if (a.mode != option_t::GenerateScoringMatrix) {
    throw std::runtime_error("something went wrong.");
  }
  LOG_DEBUG("grabbing swc filepaths for query dataset...");
  StringVector queryFilepathVector =
      getDatasetFilepaths(a.queryDatasetFilepath);
  LOG_DEBUG("query dataset size: %ld", queryFilepathVector.size());

  LOG_DEBUG("grabbing swc filepaths for target dataset...");
  StringVector targetFilepathVector =
      getDatasetFilepaths(a.targetDatasetFilepath);
  LOG_DEBUG("target dataset size: %ld", targetFilepathVector.size());

  LOG_DEBUG("getting known matches filepaths from %s",
            a.knownMatchesFilepath.c_str());
  auto [knownMatchesQueryVector, knownMatchesTargetVector] =
      getKnownMatchesFilepaths(a);
  LOG_DEBUG("known matches: query size = %ld, target size = %ld",
            knownMatchesQueryVector.size(), knownMatchesTargetVector.size());

  LOG_INFO("Pre-caching query neurons...");
  std::vector<Neuron> queryNeurons = loadNeurons(queryFilepathVector);
  LOG_INFO("Pre-caching target neurons...");
  std::vector<Neuron> targetNeurons = loadNeurons(targetFilepathVector);

  LOG_INFO("Pre-caching known match neurons...");
  std::vector<Neuron> knownQueryNeurons = loadNeurons(knownMatchesQueryVector);
  std::vector<Neuron> knownTargetNeurons = loadNeurons(knownMatchesTargetVector);

  DoubleVector distanceBins, angleBins;
  if (a.useLogBins) {
    distanceBins.assign(LOG_DISTANCE_BINS.begin(), LOG_DISTANCE_BINS.end());
    angleBins.assign(ANGLE_BINS.begin(), ANGLE_BINS.end());
  } else if (a.useRBins) {
    distanceBins.assign(R_DISTANCE_BINS.begin(), R_DISTANCE_BINS.end());
    angleBins.assign(R_ANGLE_BINS.begin(), R_ANGLE_BINS.end());
  } else {
    auto bins =
        generateBins(queryFilepathVector, targetFilepathVector,
                     knownMatchesQueryVector, knownMatchesTargetVector, 2, 5);
    distanceBins = bins.first;
    angleBins = bins.second;
  }

  Matrix knownMatrix(distanceBins, angleBins);
  Matrix randomMatrix(distanceBins, angleBins);

  // known matches: process all pairs exactly once (matches original NBLAST implementation)
  LOG_INFO("Training known match matrix from %lu pairs...", knownMatchesQueryVector.size());
  std::vector<std::pair<size_t, size_t>> knownSubset;
  knownSubset.reserve(knownMatchesQueryVector.size());
  for (size_t i = 0; i < knownMatchesQueryVector.size(); ++i) {
    knownSubset.push_back({i, i});
  }
  auto knownResults = calcDistsDotprods(knownQueryNeurons, knownTargetNeurons, knownSubset, false);
  updateMatrixWithResults(knownMatrix, knownResults);

  // random matches: sample N times for background distribution
  size_t iters = a.numGeneratorIterations;
  LOG_INFO("Training random match matrix from %lu samples...", iters);
  std::vector<std::pair<size_t, size_t>> randomSubset;
  randomSubset.reserve(iters);
  std::vector<size_t> sampledQueries;
  sampledQueries.reserve(iters);
  for (size_t i = 0; i < iters; ++i) {
    sampledQueries.push_back(queryFilepathVector.size() * drand48());
  }

  for (size_t i = 0; i < iters; ++i) {
    size_t k = sampledQueries[i];
    size_t l = targetFilepathVector.size() * drand48();
    randomSubset.push_back({k, l});
  }
  auto randomResults = calcDistsDotprods(queryNeurons, targetNeurons, randomSubset, true);
  updateMatrixWithResults(randomMatrix, randomResults);

  if (a.doDump) {
    // ensures directory out is there once (don't have to do again)
    ensureDirectory("out/knownCounts.tsv");
    std::ofstream kcout("out/knownCounts.tsv");
    kcout << knownMatrix;
    kcout.close();

    std::ofstream rcout("out/randomCounts.tsv");
    rcout << randomMatrix;
    rcout.close();
  }

  knownMatrix.toProbability();
  randomMatrix.toProbability();

  if (a.doDump) {
    std::ofstream kout("out/knownProb.txt");
    kout << knownMatrix;
    kout.close();

    std::ofstream rout("out/randomProb.txt");
    rout << randomMatrix;
    rout.close();
  }

  Matrix logLikelihoodMatrix =
      calcScoreMatrix(knownMatrix, randomMatrix, 2.0, 1e-12);

  if (!a.matrixOutfile.empty()) {
    std::ofstream mout(a.matrixOutfile);
    mout << logLikelihoodMatrix;
    mout.close();
  } else {
    std::cout << logLikelihoodMatrix;
  }
}

int run(const Args &a) {
  switch (a.mode) {
  // query two neurons for given datasets,
  // print resulting score to stdout unless specified
  case option_t::Query: {
    runQueryMode(a);
    break;
  }
  // generate a matrix for given datasets,
  // print it to stdout unless specified
  case option_t::GenerateScoringMatrix: {
    runGeneratorMode(a);
    break;
  }
  default: {
    throw std::runtime_error("argument parsing error, invalid mode");
  }
  }
  return 0;
}
