#ifndef NEURON_HPP
#define NEURON_HPP

#include "Point.hpp"
#include "utils/nanoflann.hpp"
#include <memory>
#include <string>
#include <unordered_map>

typedef nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<double, PointCloud>, PointCloud, 3>
    KDTree;

class Neuron {
public:
  // alphaK > 0 computes a per-segment alpha (NBLAST v2, Costa et al. 2016) from
  // the alphaK nearest segment midpoints, itself included; 0 disables alpha.
  Neuron(const std::string &filename, double coordScale = 1.0, int alphaK = 0);
  Neuron(Neuron &&) = default;
  Neuron(const Neuron &) = delete;
  Neuron &operator=(Neuron &&) = default;
  Neuron &operator=(const Neuron &) = delete;
  ~Neuron() = default;
  std::string getNID(void) const { return nid; }
  bool hasAlpha(void) const { return !alpha.empty(); }
  const std::vector<double> &getAlpha(void) const { return alpha; }
  PAVector nearestNeighbors(const Neuron &target, bool doSine = false,
                            bool doPrint = false) const;
  // sum of match scores of this neuron's segments against target (no normalisation)
  double directedScore(const Neuron &target, const Matrix &mat,
                       bool doSine = false) const;
  double selfScore(const Matrix &mat, bool doSine = false) const;
  double score(const Neuron &target, const Matrix &mat,
               double querySelf, double targetSelf,
               bool doSine = false, bool doPrint = false) const;
  double score(const Neuron &target, const Matrix &mat, bool doSine = false,
               bool doPrint = false) const;

private:
  std::string nid;
  std::vector<Point> pVector;
  std::unique_ptr<PointVector> mpVector;
  std::unordered_map<int, int> idToIndex;
  std::unique_ptr<PointCloud> cloud;
  std::unique_ptr<KDTree> index;
  std::vector<double> alpha; // one per segment midpoint; empty = no alpha
  void computeAlpha(int k);
  // angle measure scaled by sqrt(alpha_q * alpha_t) when both carry alpha
  double weightedAngle(const Neuron &target, size_t qi, size_t ti,
                       double angle) const;
};

#endif // NEURON_HPP
