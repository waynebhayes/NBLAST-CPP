#include "Neuron.hpp"
#include "io/FileIO.hpp"
#include "utils/StringUtils.hpp"
#include "utils/nanoflann.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

Neuron::Neuron(const std::string &filepath, double coordScale, int alphaK) {
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
  if (alphaK > 0)
    computeAlpha(alphaK);
}

// Eigenvalues of a symmetric 3x3 matrix, descending (closed form; Smith 1961).
static void symEig3(const double A[3][3], double ev[3]) {
  double p1 = A[0][1] * A[0][1] + A[0][2] * A[0][2] + A[1][2] * A[1][2];
  double q = (A[0][0] + A[1][1] + A[2][2]) / 3.0;
  if (p1 == 0) {
    ev[0] = A[0][0];
    ev[1] = A[1][1];
    ev[2] = A[2][2];
  } else {
    double p2 = (A[0][0] - q) * (A[0][0] - q) + (A[1][1] - q) * (A[1][1] - q) +
                (A[2][2] - q) * (A[2][2] - q) + 2.0 * p1;
    double p = std::sqrt(p2 / 6.0);
    double B[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        B[i][j] = (A[i][j] - (i == j ? q : 0.0)) / p;
    double r = (B[0][0] * (B[1][1] * B[2][2] - B[1][2] * B[2][1]) -
                B[0][1] * (B[1][0] * B[2][2] - B[1][2] * B[2][0]) +
                B[0][2] * (B[1][0] * B[2][1] - B[1][1] * B[2][0])) /
               2.0;
    double phi =
        r <= -1 ? std::numbers::pi / 3.0 : (r >= 1 ? 0.0 : std::acos(r) / 3.0);
    ev[0] = q + 2.0 * p * std::cos(phi);
    ev[2] = q + 2.0 * p * std::cos(phi + 2.0 * std::numbers::pi / 3.0);
    ev[1] = 3.0 * q - ev[0] - ev[2];
  }
  std::sort(ev, ev + 3, [](double a, double b) { return a > b; });
}

// Same definition as nat::dotprops: for each point, take its k nearest
// neighbours (itself included), and with l1 >= l2 >= l3 the eigenvalues of
// their scatter matrix, alpha = (l1 - l2) / (l1 + l2 + l3). Near 1 where the
// neighbourhood is linear (axon-like), near 0 where it is isotropic.
void Neuron::computeAlpha(int k) {
  const PointVector &mids = *mpVector;
  size_t n = mids.size();
  alpha.assign(n, 0.0);
  size_t kk = std::min<size_t>(static_cast<size_t>(k), n);
  if (kk < 2)
    return;
  std::vector<size_t> idx(kk);
  std::vector<double> d2(kk);
  for (size_t i = 0; i < n; ++i) {
    double q[3] = {mids[i].x, mids[i].y, mids[i].z};
    nanoflann::KNNResultSet<double> resultSet(kk);
    resultSet.init(idx.data(), d2.data());
    index->findNeighbors(resultSet, q, nanoflann::SearchParameters());
    size_t found = resultSet.size();
    double m[3] = {0, 0, 0};
    for (size_t j = 0; j < found; ++j) {
      m[0] += mids[idx[j]].x;
      m[1] += mids[idx[j]].y;
      m[2] += mids[idx[j]].z;
    }
    for (double &v : m)
      v /= static_cast<double>(found);
    double S[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    for (size_t j = 0; j < found; ++j) {
      double c[3] = {mids[idx[j]].x - m[0], mids[idx[j]].y - m[1],
                     mids[idx[j]].z - m[2]};
      for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b)
          S[a][b] += c[a] * c[b];
    }
    double ev[3];
    symEig3(S, ev);
    double sum = ev[0] + ev[1] + ev[2];
    alpha[i] = sum > 0 ? (ev[0] - ev[1]) / sum : 0.0;
  }
}

double Neuron::weightedAngle(const Neuron &target, size_t qi, size_t ti,
                             double angle) const {
  if (alpha.empty() || target.alpha.empty())
    return angle;
  return angle * std::sqrt(alpha[qi] * target.alpha[ti]);
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

    // angle measure (alpha-weighted when both neurons carry alpha)
    double angleMeasure =
        weightedAngle(target, i, nearestIdx, r_i.angleMeasure(s_i, doSine));

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
  const std::vector<Point> &queryMids = *this->mpVector;
  for (size_t i = 0; i < queryMids.size(); ++i) {
    const Point &qi = queryMids[i];
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
    total += mat.score(
        std::sqrt(outDistanceSqr),
        weightedAngle(target, i, nearestIdx, r_i.angleMeasure(s_i, doSine)));
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
