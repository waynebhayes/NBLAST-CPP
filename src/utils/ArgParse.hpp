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
    "  -S <factor>                      Multiply SWC coordinates by <factor> on load\n"
    "                                   (e.g. 0.001 converts nm to um; default: 1).\n"
    "  -b<N>n | -b<P>g                  Output only the best-scoring pairs: the top N\n"
    "                                   pairs (e.g. -b10n) or the top P percent\n"
    "                                   (e.g. -b10g = top 10%). Sorted by score,\n"
    "                                   descending. All-by-all counts each unordered\n"
    "                                   pair once and skips self-pairs.\n"
    "  -b<K>q                           With -a: compare every query neuron against every\n"
    "                                   target neuron (-i <query_dir>,<target_dir>) and\n"
    "                                   keep the top K targets for each query. Query IDs\n"
    "                                   come from stdin (all of query_dir if empty).\n"
    "                                   Memory stays bounded; set NBLAST_SLICE=<n> to\n"
    "                                   change queries processed per slice (default 2000).\n"
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

enum class BestKind : int { None, Count, Percent, PerQuery };

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
  double coordScale = 1.0;
  BestKind bestKind = BestKind::None;
  double bestValue = 0.0; // pairs for Count, percent in (0,100] for Percent, K for PerQuery
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
