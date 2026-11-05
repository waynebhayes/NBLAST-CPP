#include "Neuron.hpp"
#include "io/FileIO.hpp"
#include "utils/StringUtils.hpp"
#include "utils/nanoflann.hpp"
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

Neuron::Neuron(const std::string &filepath, double coordScale) {
  basenameNoExt(filepath, nid);
  pVector = loadPoints(filepath, coordScale);
  for (int i = 0; i < (int)pVector.size(); ++i) {
    idToIndex[pVector[i].id] = i;
  }
  mpVector = std::make_unique<PointVector>();
  mpVector->reserve(pVector.size());
  for (const auto &pt : pVector) {
    if (pt.parent == -1)
      continue;
    if (idToIndex.find(pt.parent) == idToIndex.end()) {
      throw std::runtime_error("Parent ID " + std::to_string(pt.parent) +
                               " not found for point " + std::to_string(pt.id));
    }
    int internalParentIndex = idToIndex.at(pt.parent);
    const Point &parentPt = pVector[internalParentIndex];
    Point tangent = parentPt - pt;
    Point pos = pt.midpoint(parentPt);

    mpVector->emplace_back(pt.id, pos.x, pos.y, pos.z, internalParentIndex,
                           tangent.x, tangent.y, tangent.z);
  }

  cloud = std::make_unique<PointCloud>(*mpVector);
  index = std::make_unique<KDTree>(
      3, *cloud, nanoflann::KDTreeSingleIndexAdaptorParams(10));
  index->buildIndex();
}

PAVector Neuron::nearestNeighbors(const Neuron &target, bool doSine,
                                  bool doPrint) const {
  PAVector matchVector;
  matchVector.reserve(this->mpVector->size());
  // a target with no segments has nothing to match against
  if (target.mpVector->empty())
    return matchVector;
  // For each query midpoint, perform nearest neighbor search
  for (size_t i = 0; i < this->mpVector->size(); ++i) {
    const Point &qi = (*this->mpVector)[i];
    double query_pt[3] = {qi.x, qi.y, qi.z};

    size_t nearestIdx = 0;
    double outDistanceSqr = 0;

    nanoflann::KNNResultSet<double> resultSet(1);
    resultSet.init(&nearestIdx, &outDistanceSqr);
    target.index->findNeighbors(resultSet, query_pt,
                                nanoflann::SearchParameters());

    const Point &ti = (*target.mpVector)[nearestIdx];

    // Segment vectors (stored in tx, ty, tz of midpoints)
    Point r_i(qi.id, qi.tx, qi.ty, qi.tz, -1);
    Point s_i(ti.id, ti.tx, ti.ty, ti.tz, -1);

    // angle measure
    double angleMeasure = r_i.angleMeasure(s_i, doSine);

    // output: id_i id_j distance angle
    PointAlignment pc{qi.id, ti.id, std::sqrt(outDistanceSqr), angleMeasure};
    matchVector.push_back(pc);
    if (doPrint) {
      pc.printDifference(std::cout);
    }
  }
  return matchVector;
}

static void computeRawScores(const Matrix &mat, PAVector &matchVector) {
  for (PointAlignment &pm : matchVector) {
    if (pm.queryPointID == -1 || pm.targetPointID == -1) {
      continue;
    }
    pm.computeRawScore(mat);
  }
}

static double sumRawScores(const PAVector &vec) {
  double res = 0;
  for (const PointAlignment &elem : vec) {
    res += elem.score;
  }
  return res;
}

// Same arithmetic as nearestNeighbors + computeRawScores + sumRawScores (same
// summation order, so results are bit-identical) but without materialising the
// per-segment match vector. Distances and angle measures are non-negative by
// construction, so the validity check in computeRawScore is not needed here.
double Neuron::directedScore(const Neuron &target, const Matrix &mat,
                             bool doSine) const {
  double total = 0;
  const std::vector<Point> &targetMids = *target.mpVector;
  // a target with no segments has nothing to match against
  if (targetMids.empty())
    return 0;
  for (const Point &qi : *this->mpVector) {
    double query_pt[3] = {qi.x, qi.y, qi.z};
    size_t nearestIdx = 0;
    double outDistanceSqr = 0;
    nanoflann::KNNResultSet<double> resultSet(1);
    resultSet.init(&nearestIdx, &outDistanceSqr);
    target.index->findNeighbors(resultSet, query_pt,
                                nanoflann::SearchParameters());
    const Point &ti = targetMids[nearestIdx];
    Point r_i(qi.id, qi.tx, qi.ty, qi.tz, -1);
    Point s_i(ti.id, ti.tx, ti.ty, ti.tz, -1);
    total += mat.score(std::sqrt(outDistanceSqr), r_i.angleMeasure(s_i, doSine));
  }
  return total;
}

double Neuron::selfScore(const Matrix &mat, bool doSine) const {
  return directedScore(*this, mat, doSine);
}

double Neuron::score(const Neuron &target, const Matrix &mat,
                     double querySelf, double targetSelf,
                     bool doSine, bool doPrint) const {
  double forwardTotal, reverseTotal;
  if (doPrint) {
    PAVector forwardMatchVector = this->nearestNeighbors(target, doSine, doPrint);
    computeRawScores(mat, forwardMatchVector);
    forwardTotal = sumRawScores(forwardMatchVector);

    PAVector reverseMatchVector = target.nearestNeighbors(*this, doSine, doPrint);
    computeRawScores(mat, reverseMatchVector);
    reverseTotal = sumRawScores(reverseMatchVector);
  } else {
    forwardTotal = directedScore(target, mat, doSine);
    reverseTotal = target.directedScore(*this, mat, doSine);
  }

  double normalizedForward = (querySelf != 0) ? (forwardTotal / querySelf) : 0;
  double normalizedReverse = (targetSelf != 0) ? (reverseTotal / targetSelf) : 0;
  return (normalizedForward + normalizedReverse) / 2.0;
}

double Neuron::score(const Neuron &target, const Matrix &mat, bool doSine,
                     bool doPrint) const {
  double qs = selfScore(mat, doSine);
  double ts = target.selfScore(mat, doSine);
  return score(target, mat, qs, ts, doSine, doPrint);
}
