#include "MatrixIO.hpp"
#include "core/Matrix.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace MatrixIO {

Matrix loadMatrixFromTSV(const std::string &filepath) {
  std::ifstream fin(filepath);
  if (!fin.is_open()) {
    throw std::runtime_error("Cannot open " + filepath);
  }
  Matrix m;

  std::string line;
  bool isHeader = true;

  while (std::getline(fin, line)) {
    std::stringstream ss(line);
    std::string cell;
    // store cells
    std::vector<std::string> parts;

    while (std::getline(ss, cell, '\t'))
      parts.push_back(cell);

    // angle bin
    if (isHeader) {
      isHeader = false;
      for (size_t i = 1; i < parts.size(); ++i) {
        // cos_0.1 (if sin will use the same format?)
        auto position = parts[i].find('_');
        m.getAngleBins().push_back(std::stod(parts[i].substr(position + 1)));
      }
      continue;
    }

    // distance bin
    double upperDst = std::stod(parts[0]);
    m.getDistanceBins().push_back(upperDst);

    std::vector<double> row;
    for (size_t i = 1; i < parts.size(); ++i)
      row.push_back(std::stod(parts[i]));

    m.getTable().push_back(row);
  }

  return m;
}
} // namespace MatrixIO
