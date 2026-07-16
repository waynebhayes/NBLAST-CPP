#include "FileIO.hpp"
#include "core/Point.hpp"
#include "utils/ArgParse.hpp"
#include "utils/Logging.hpp"
#include "utils/StringUtils.hpp"

#include <filesystem>
#include <fstream>
#include <string>

PointVector loadPoints(const std::string &filepath) {
  std::ifstream fin{filepath};
  if (!fin) {
    throw std::runtime_error("Cannot open " + filepath);
  }
  PointVector vec;
  std::string line;
  while (std::getline(fin, line)) {
    if (line.empty())
      continue;
    if (!line.empty() && line[0] == '#')
      continue;
    Point p;
    p.parse(line);
    vec.push_back(p);
  }
  fin.close();
  return vec;
}

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

std::vector<Neuron>
loadNeurons(const std::vector<std::string> &neuronFilepaths) {
  std::vector<Neuron> neuronVector;
  neuronVector.reserve(neuronFilepaths.size());
  for (size_t i = 0; i < neuronFilepaths.size(); ++i) {
    neuronVector.emplace_back(neuronFilepaths.at(i));
  }
  return neuronVector;
}

void ensureDirectory(const std::string &filepath) {
  namespace fs = std::filesystem;

  fs::path p(filepath);
  if (p.has_parent_path()) {
    fs::create_directories(p.parent_path());
  }
}

StringVector getDatasetFilepaths(const std::string &filepath) {
  namespace fs = std::filesystem;
  std::vector<std::string> pathVector;
  for (auto const &dir_entry : fs::directory_iterator{filepath}) {
    pathVector.push_back(dir_entry.path().string());
  }
  return pathVector;
}

StringVectorPair getKnownMatchesFilepaths(const Args &a) {
  StringVectorPair vecPair;
  std::ifstream fin{a.knownMatchesFilepath, std::ios::in};
  if (!fin) {
    throw std::runtime_error("Cannot open " + a.knownMatchesFilepath);
  }

  std::string line, v1, v2;
  std::getline(fin, line); // ignore header

  std::vector<std::string> singleList;
  while (std::getline(fin, line)) {
    if (line.empty())
      continue;
    std::istringstream sin(line);
    if (!(sin >> v1))
      continue;

    if (!(sin >> v2)) {
      // single neuron per line: collect for all-by-all
      singleList.push_back(v1);
    } else {
      // pair per line: add directly
      vecPair.first.push_back(
          filenameToPath(a.queryDatasetFilepath, v1, ".swc"));
      vecPair.second.push_back(
          filenameToPath(a.targetDatasetFilepath, v2, ".swc"));
    }
  }

  if (!singleList.empty()) {
    LOG_INFO("Generating %lu all-by-all pairs from single-column ground truth...",
             (uint64_t)singleList.size() * (singleList.size() - 1));
    for (const auto &qID : singleList) {
      for (const auto &tID : singleList) {
        if (qID != tID) {
          vecPair.first.push_back(
              filenameToPath(a.queryDatasetFilepath, qID, ".swc"));
          vecPair.second.push_back(
              filenameToPath(a.targetDatasetFilepath, tID, ".swc"));
        }
      }
    }
  }

  return vecPair;
}
