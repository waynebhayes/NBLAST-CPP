#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <cstdint>
#include <string>
#include <vector>

constexpr const char *USAGE_MSG =
    "USAGE: nblast++ <mode> [options]\n"
    "\n"
    "MODES:\n"
    "  -q <scoring_matrix.tsv>          Query mode: score neuron pairs.\n"
    "                                   Reads query/target ID pairs from "
    "stdin.\n"
    "  -g <known_matches.tsv>,<N>       Generator mode: build a scoring "
    "matrix\n"
    "                                   from N iterations using a "
    "known-matches file.\n"
    "\n"
    "OPTIONS:\n"
    "  -i <query_dir>[,<target_dir>]    Input dataset directories (required).\n"
    "  -o <outfile>                     Write scoring matrix to file (default: "
    "stdout).\n"
    "  -a                               All-by-all query (query mode only).\n"
      "  -s                               Use sine for angle measure (default: "
    "cosine).\n"
    "  -I                               Enable bilinear interpolation in "
    "scoring (default: off).\n"
    "  -r                               Use R (nat.nblast) default distance "
    "bins.\n"
    "  -l                               Use log-spaced FlyCircuit bins "
    "(0.25-64 um, factor 2).\n"
    "  -d                               Dump intermediate files to ./out/.\n"
    "  -h                               Print this message and exit.\n";

void printUsage(std::ostream &out);

enum class option_t : int {
  Query,
  AllByAll,
  GenerateScoringMatrix,
  DefaultMode
};
std::ostream &operator<<(std::ostream &out, option_t op);
std::string optToString(option_t m);

using StringVector = std::vector<std::string>;
struct Args {
  StringVector positionalArgs;
  std::string matrixFilepath;
  std::string knownMatchesFilepath;
  std::string queryDatasetFilepath;
  std::string targetDatasetFilepath;
  std::string matrixOutfile;
  option_t mode = option_t::DefaultMode;
  uint64_t numGeneratorIterations = 0;
  bool doSine = false;
  bool doDump = false;
  bool doAllByAll = false;
  bool doInterpolation = false;
  bool useRBins = false;
  bool useLogBins = false;

  friend std::ostream &operator<<(std::ostream &out, const Args &a);
};

Args parseArgs(int argc, char *argv[]);

#endif // ARGPARSE_HPP
