#ifndef FILEIO_HPP
#define FILEIO_HPP

#include "core/Neuron.hpp"
#include "core/Point.hpp"
#include "utils/ArgParse.hpp"

#include <string>
#include <vector>

PointVector loadPoints(const std::string &filepath);
Matrix loadMatrixFromTSV(const std::string &filepath);

std::vector<Neuron>
loadNeurons(const std::vector<std::string> &neuronFilepaths);

void ensureDirectory(const std::string &path);

using StringVector = std::vector<std::string>;
StringVector getDatasetFilepaths(const std::string &filepath);

using StringVectorPair =
    std::pair<std::vector<std::string>, std::vector<std::string>>;
StringVectorPair getKnownMatchesFilepaths(const Args &a);

#endif // FILEIO_HPP
