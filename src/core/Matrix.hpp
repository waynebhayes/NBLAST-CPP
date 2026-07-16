#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <array>
#include <iostream>
#include <vector>

// defaults for banc-fafb
static constexpr unsigned int NUM_DISTANCE_BINS = 7;
static constexpr unsigned int NUM_ANGLE_BINS = 10;
static constexpr std::array<double, NUM_DISTANCE_BINS> DISTANCE_BINS{
    20000, 30000, 45000, 65000, 90000, 120000, 160000};
static constexpr std::array<double, NUM_ANGLE_BINS> ANGLE_BINS{
    .1, .2, .3, .4, .5, .6, .7, .8, .9, 1};

static constexpr unsigned int NUM_R_DISTANCE_BINS = 21;
static constexpr unsigned int NUM_R_ANGLE_BINS = 10;
static constexpr std::array<double, NUM_R_DISTANCE_BINS> R_DISTANCE_BINS{
    0.75, 1.5, 2, 2.5, 3, 3.5, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 20, 25, 30, 40, 500};
static constexpr std::array<double, NUM_R_ANGLE_BINS> R_ANGLE_BINS{
    .1, .2, .3, .4, .5, .6, .7, .8, .9, 1};

// log-spaced factor-2 bins for FlyCircuit (µm scale), matching exp/log_smat10000.tsv
static constexpr unsigned int NUM_LOG_DISTANCE_BINS = 9;
static constexpr std::array<double, NUM_LOG_DISTANCE_BINS> LOG_DISTANCE_BINS{
    0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0};


using DoubleVector = std::vector<double>;
using DoubleVector2D = std::vector<DoubleVector>;

class Matrix {
public:
  Matrix() : distanceBins(), angleBins(), table() {}
  Matrix(const DoubleVector &distanceBins, const DoubleVector &angleBins)
      : distanceBins(distanceBins.begin(), distanceBins.end()),
        angleBins(angleBins.begin(), angleBins.end()),
        table(distanceBins.size(), DoubleVector(angleBins.size(), 0.0)) {}
  void increment(double distance, double angle, double value = 1.0);
  Matrix &prefixSum();
  Matrix &toECDF();
  Matrix &toProbability();
  double totalCount() const;
  double score(double distance, double angle) const;
  friend std::ostream &operator<<(std::ostream &out, const Matrix &mat);
  inline DoubleVector &getDistanceBins() { return distanceBins; }
  inline const DoubleVector &getDistanceBins() const { return distanceBins; }
  inline DoubleVector &getAngleBins() { return angleBins; }
  inline const DoubleVector &getAngleBins() const { return angleBins; }
  inline DoubleVector2D &getTable() { return table; }
  inline const DoubleVector2D &getTable() const { return table; }
  inline void setInterpolate(bool val) { interpolate = val; }
  inline bool getInterpolate() const { return interpolate; }

private:
  DoubleVector distanceBins;
  DoubleVector angleBins;
  DoubleVector2D table;
  bool interpolate = true;

  int findDistanceBin(double value) const;

  int findAngleBin(double value) const;
};

#endif // MATRIX_HPP
