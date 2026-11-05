#include "Runner.hpp"
#include "analysis/Pipeline.hpp"
#include "core/Matrix.hpp"
#include "io/FileIO.hpp"
#include "utils/ArgParse.hpp"
#include "utils/Logging.hpp"
#include "utils/StringUtils.hpp"
#include "utils/Timer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

// Every query neuron vs every target neuron, keeping the top K targets per query.
// Targets are loaded once; queries go through in slices and each slice's rows
// are printed as soon as it finishes, so memory stays bounded and a long run
// shows progress.
static void runCrossTopK(const Args &a, const Matrix &mat) {
  std::vector<std::string> queryPaths;
  std::string id;
  while (std::cin >> id)
    queryPaths.push_back(filenameToPath(a.queryDatasetFilepath, id, ".swc"));
  if (queryPaths.empty()) {
    queryPaths = getDatasetFilepaths(a.queryDatasetFilepath);
    std::sort(queryPaths.begin(), queryPaths.end());
  }
  std::vector<std::string> targetPaths = getDatasetFilepaths(a.targetDatasetFilepath);
  std::sort(targetPaths.begin(), targetPaths.end());

  size_t slice = 2000;
  if (const char *env = std::getenv("NBLAST_SLICE"))
    if (long v = std::atol(env); v > 0)
      slice = (size_t)v;
  const size_t k = (size_t)a.bestValue;

  LOG_INFO("Loading %zu target neurons...", targetPaths.size());
  std::vector<Neuron> targets = loadNeurons(targetPaths, a.coordScale);
  std::vector<double> targetSelf = allSelfScores(mat, targets);

  for (size_t s0 = 0; s0 < queryPaths.size(); s0 += slice) {
    size_t s1 = std::min(s0 + slice, queryPaths.size());
    std::vector<std::string> paths(queryPaths.begin() + s0, queryPaths.begin() + s1);
    std::vector<Neuron> queries = loadNeurons(paths, a.coordScale);
    std::vector<double> querySelf = allSelfScores(mat, queries);
    auto hits = topKPerQuery(mat, queries, querySelf, targets, targetSelf, k);
    for (size_t q = 0; q < queries.size(); ++q)
      for (const Hit &h : hits[q])
        std::cout << queries[q].getNID() << '\t' << targets[h.target].getNID()
                  << '\t' << h.score << '\n';
    std::cout.flush();
    LOG_INFO("Queries done: %zu / %zu", s1, queryPaths.size());
  }
}

void runQueryMode(const Args &a) {
  LOG_INFO("Using Scoring Matrix: \"%s\"", a.matrixFilepath.c_str());

  Matrix mat = loadMatrixFromTSV(a.matrixFilepath);
  mat.setInterpolate(a.doInterpolation);
  TimerStats ts;
  if (a.bestKind == BestKind::PerQuery) {
    runCrossTopK(a, mat);
    return;
  }
  if (a.positionalArgs.empty()) {
    if (a.doAllByAll) {
      std::vector<std::string> neuronFilepaths;
      std::string neuronID;
      while (std::cin >> neuronID) {
        std::string neuronFilepath =
            filenameToPath(a.queryDatasetFilepath, neuronID, ".swc");
        neuronFilepaths.push_back(neuronFilepath);
      }
      // phase timings go to stderr when NBLAST_TIMING is set (see scripts/timing)
      const bool timing = std::getenv("NBLAST_TIMING") != nullptr;
      auto t0 = std::chrono::steady_clock::now();
      std::vector<Neuron> neurons = loadNeurons(neuronFilepaths, a.coordScale);
      auto t1 = std::chrono::steady_clock::now();
      std::vector<Score> scoreVector =
          a.bestKind != BestKind::None
              ? allByAllBest(mat, neurons, a.bestKind, a.bestValue)
              : allByAll(mat, neurons);
      auto t2 = std::chrono::steady_clock::now();
      if (timing) {
        std::cerr << "TIMING\tprep\t"
                  << std::chrono::duration<double>(t1 - t0).count() << "\n"
                  << "TIMING\tscore\t"
                  << std::chrono::duration<double>(t2 - t1).count() << "\n";
      }
      for (const Score &s : scoreVector) {
        s.print(std::cout, "\t");
      }
      std::cout.flush();
      return;
    } else {
      // each distinct neuron file is loaded once, however many pairs use it
      std::vector<std::string> neuronFilepaths;
      std::unordered_map<std::string, size_t> indexOf;
      auto intern = [&](const std::string &path) {
        auto [it, inserted] = indexOf.emplace(path, neuronFilepaths.size());
        if (inserted)
          neuronFilepaths.push_back(path);
        return it->second;
      };
      std::vector<std::pair<size_t, size_t>> pairs;
      std::string queryID, targetID;
      while (std::cin >> queryID >> targetID) {
        size_t qi = intern(filenameToPath(a.queryDatasetFilepath, queryID, ".swc"));
        size_t ti = intern(filenameToPath(a.targetDatasetFilepath, targetID, ".swc"));
        pairs.emplace_back(qi, ti);
      }
      std::vector<Neuron> neurons = loadNeurons(neuronFilepaths, a.coordScale);
      std::vector<Score> scoreVector =
          timeFunction(ts, [&]() {
            return a.bestKind != BestKind::None
                       ? queryPairsBest(mat, neurons, pairs, a.bestKind, a.bestValue)
                       : queryPairs(mat, neurons, pairs);
          });

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
  Neuron queryNeuron = Neuron(queryFilepath, a.coordScale);
  std::vector<std::string> targetFilepaths;
  for (size_t i = 1; i < a.positionalArgs.size(); i++) {
    std::string targetFilepath =
        filenameToPath(a.targetDatasetFilepath, a.positionalArgs[i], ".swc");
    targetFilepaths.push_back(targetFilepath);
  }
  std::vector<Neuron> targetNeurons = loadNeurons(targetFilepaths, a.coordScale);
  std::vector<Score> scoreVector;
  scoreVector.reserve(targetNeurons.size());
  for (size_t j = 0; j < targetNeurons.size(); j++) {
    scoreVector.push_back(query(mat, queryNeuron, targetNeurons[j]));
  }
  if (a.bestKind != BestKind::None)
    keepBest(scoreVector, a.bestKind, a.bestValue);
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
  std::vector<Neuron> queryNeurons = loadNeurons(queryFilepathVector, a.coordScale);
  LOG_INFO("Pre-caching target neurons...");
  std::vector<Neuron> targetNeurons = loadNeurons(targetFilepathVector, a.coordScale);

  LOG_INFO("Pre-caching known match neurons...");
  std::vector<Neuron> knownQueryNeurons = loadNeurons(knownMatchesQueryVector, a.coordScale);
  std::vector<Neuron> knownTargetNeurons = loadNeurons(knownMatchesTargetVector, a.coordScale);

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
                     knownMatchesQueryVector, knownMatchesTargetVector, 2, 5,
                     a.coordScale);
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
