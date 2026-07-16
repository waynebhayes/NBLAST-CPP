#include "Matrix.hpp"
#include "utils/Logging.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

void Matrix::increment(double distance, double angle, double value) {
  int row = findDistanceBin(distance);
  int col = findAngleBin(angle);
  LOG_DEBUG("row: %d column: %d", row, col);
  LOG_DEBUG("value: %f", value);
  LOG_DEBUG("result: %f", value + table[row][col]);
  table[row][col] += value;
}
Matrix &Matrix::prefixSum() {
  for (size_t i = 0; i < table.size(); ++i) {
    double tmp = 0.0, row_counter = 0.0;
    for (size_t j = 0; j < table[i].size(); ++j) {
      tmp = table[i][j];
      table[i][j] += row_counter;
      row_counter += tmp;
      if (i > 0) {
        table[i][j] += table[i - 1][j];
      }
    }
  }
  return *this;
}
Matrix &Matrix::toECDF() {
  if (table.empty() || table[0].empty())
    throw std::runtime_error("cannot convert to ECDF: matrix empty/invalid");

  double total = table.back().back();
  if (total == 0.0)
    throw std::runtime_error(
        "cannot convert to ECDF: total sum of matrix is zero");

  for (size_t i = 0; i < table.size(); ++i) {
    for (size_t j = 0; j < table[i].size(); ++j) {
      table[i][j] /= total;
    }
  }
  return *this;
}

double Matrix::totalCount() const {
  double total = 0.0;
  for (size_t i = 0; i < table.size(); ++i) {
    for (size_t j = 0; j < table[i].size(); ++j) {
      total += table[i][j];
    }
  }
  return total;
}

Matrix &Matrix::toProbability() {
  if (table.empty() || table[0].empty())
    throw std::runtime_error(
        "cannot convert to probability: matrix empty/invalid");

  double total = totalCount();
  if (total == 0.0)
    throw std::runtime_error(
        "cannot convert to probability: total sum of matrix is zero");

  for (size_t i = 0; i < table.size(); ++i) {
    for (size_t j = 0; j < table[i].size(); ++j) {
      table[i][j] /= total;
    }
  }
  return *this;
}

double Matrix::score(double distance, double angle) const {
  if (table.empty() || table[0].empty())
    return 0.0;

  auto itDist =
      std::lower_bound(distanceBins.begin(), distanceBins.end(), distance);
  auto itAngle = std::lower_bound(angleBins.begin(), angleBins.end(), angle);

  size_t i1 = std::distance(distanceBins.begin(), itDist);
  size_t j1 = std::distance(angleBins.begin(), itAngle);

  if (i1 >= distanceBins.size())
    i1 = distanceBins.size() - 1;
  if (j1 >= angleBins.size())
    j1 = angleBins.size() - 1;

  if (i1 == 0 || j1 == 0 || itDist == distanceBins.end() ||
      itAngle == angleBins.end() || !interpolate) {
    return table[i1][j1];
  }

  size_t i0 = i1 - 1;
  size_t j0 = j1 - 1;

  double x0 = distanceBins[i0];
  double x1 = distanceBins[i1];
  double y0 = angleBins[j0];
  double y1 = angleBins[j1];

  double f00 = table[i0][j0];
  double f01 = table[i0][j1];
  double f10 = table[i1][j0];
  double f11 = table[i1][j1];

  double dx = (distance - x0) / (x1 - x0);
  double dy = (angle - y0) / (y1 - y0);

  double fx0 = f00 + dx * (f10 - f00);
  double fx1 = f01 + dx * (f11 - f01);

  return fx0 + dy * (fx1 - fx0);
}
std::ostream &operator<<(std::ostream &out, const Matrix &mat) {
  constexpr int precision = 4;

  std::ios oldState(nullptr);
  oldState.copyfmt(out);

  out << "dist/angle";
  out << "\t";
  for (size_t i = 0; i < mat.angleBins.size(); ++i) {
    out << "cos_" << mat.angleBins[i];
    if (i + 1 < mat.angleBins.size())
      out << "\t";
  }
  out << "\n";

  for (size_t j = 0; j < mat.table.size(); ++j) {
    out << std::fixed << std::setprecision(2);
    out << mat.distanceBins[j] << "\t";
    out << std::fixed << std::setprecision(precision);

    for (size_t k = 0; k < mat.table[j].size(); ++k) {
      out << mat.table[j][k];
      if (k + 1 < mat.table[j].size())
        out << "\t";
    }
    out << "\n";
  }

  out.copyfmt(oldState);

  return out;
}

int Matrix::findDistanceBin(double value) const {
  auto it = std::lower_bound(distanceBins.begin(), distanceBins.end(), value);
  if (it == distanceBins.end())
    return distanceBins.size() - 1;
  return std::distance(distanceBins.begin(), it);
}
int Matrix::findAngleBin(double value) const {
  auto it = std::lower_bound(angleBins.begin(), angleBins.end(), value);
  if (it == angleBins.end())
    return angleBins.size() - 1;
  return std::distance(angleBins.begin(), it);
}
