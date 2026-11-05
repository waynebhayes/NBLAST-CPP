#include "ArgParse.hpp"
#include "StringUtils.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <unistd.h>

extern int optind;

void printUsage(std::ostream &out) { out << USAGE_MSG; }

std::ostream &operator<<(std::ostream &out, option_t op) {
  switch (op) {
  case option_t::Query:
    out << "q";
    break;
  case option_t::AllByAll:
    out << "a";
    break;
  case option_t::GenerateScoringMatrix:
    out << "g";
    break;
  case option_t::DefaultMode:
    out << "default";
    break;
  default:
    out << "unknown";
    break;
  }
  return out;
}

std::string optToString(option_t m) {
  switch (m) {
  case option_t::Query:
    return "q";
  case option_t::AllByAll:
    return "a";
  case option_t::GenerateScoringMatrix:
    return "g";
  case option_t::DefaultMode:
    return "default";
  default:
    return "unknown";
  }
}

std::ostream &operator<<(std::ostream &out, const Args &a) {
  out << "optind: " << optind << '\n'
      << "matrixFilepath: " << a.matrixFilepath << '\n'
      << "knownMatchesFilepath: " << a.knownMatchesFilepath << '\n'
      << "queryDatasetFilepath: " << a.queryDatasetFilepath << '\n'
      << "targetDatasetFilepath: " << a.targetDatasetFilepath << '\n'
      << "mode: " << a.mode << '\n'
      << "numGeneratorIterations: " << a.numGeneratorIterations << '\n'
      << "coordScale: " << a.coordScale << '\n'
      << "bestKind: " << static_cast<int>(a.bestKind) << '\n'
      << "bestValue: " << a.bestValue << '\n'
      << "doSine: " << a.doSine << '\n'
      << "doDump: " << a.doDump << '\n'
      << "doAllByAll: " << a.doAllByAll << '\n'
      << "doInterpolation: " << a.doInterpolation << '\n'
      << "useRBins: " << a.useRBins << '\n'
      << "useLogBins: " << a.useLogBins;
  return out;
}

void setMode(Args &a, option_t next) {
  if (a.mode != option_t::DefaultMode) {
    throw std::runtime_error("Invalid combination: " + optToString(a.mode) +
                             " and " + optToString(next));
  }
  a.mode = next;
}

Args parseArgs(int argc, char *argv[]) {
  if (argc == 1) {
    printUsage(std::cout);
    exit(EXIT_SUCCESS);
  }
  Args a;
  int opt = 0;
  bool optIProvided = false;
  while ((opt = getopt(argc, argv, ":hq:g:i:o:S:b:sdaIrl")) != -1) {
    switch (opt) {
    // print usage
    case 'h': {
      printUsage(std::cout);
      exit(EXIT_SUCCESS);
    }
    // ===== main modes =====
    // query toolchain, compares one query .swc file to one or more target .swc
    // files
    case 'q': {
      setMode(a, option_t::Query);
      a.matrixFilepath = optarg;
      if (a.matrixFilepath.empty()) {
        throw std::runtime_error("matrixFilepath cannot be empty");
      }
      break;
    }
    // generator toolchain, generates match and random p-value matrices
    // given .swc files and known matches file
    case 'g': {
      setMode(a, option_t::GenerateScoringMatrix);

      // comma separated arg
      std::pair<std::string, std::string> res;
      int rc = splitOnComma(optarg, res);
      if (rc) {
        throw std::runtime_error("argument for -g invalid");
      }

      if (res.first.empty()) {
        throw std::runtime_error("filepath in -g option empty");
      }
      uint64_t numItersUint;
      rc = stringToUInt(res.second, numItersUint);
      if (rc == -1) {
        throw std::runtime_error(
            "numGeneratorIterations must be an unsigned integer");
      } else if (rc == -2) {
        throw std::runtime_error("numGeneratorIterations out of range");
      }
      a.knownMatchesFilepath = res.first;
      a.numGeneratorIterations = numItersUint;

      break;
    }
    // ===== options =====
    // input directories
    case 'i': {
      optIProvided = true;
      std::pair<std::string, std::string> res;
      int rc = splitOnComma(optarg, res);
      if (rc == -1) {
        // No comma: use same directory for both query and target
        a.queryDatasetFilepath = optarg;
        a.targetDatasetFilepath = optarg;
      } else {
        if (res.first.empty()) {
          throw std::runtime_error("query dataset filepath empty");
        } else if (res.second.empty()) {
          throw std::runtime_error("target dataset filepath empty");
        }
        a.queryDatasetFilepath = res.first;
        a.targetDatasetFilepath = res.second;
      }
      break;
    }
    case 'S': {
      size_t pos = 0;
      double scale = 0.0;
      try {
        scale = std::stod(optarg, &pos);
      } catch (const std::exception &) {
        pos = 0;
      }
      if (pos == 0 || optarg[pos] != '\0' || !(scale > 0.0)) {
        throw std::runtime_error("argument for -S must be a positive number");
      }
      a.coordScale = scale;
      break;
    }
    // best-scoring pairs only: <N>n (top N pairs) or <P>g (top P percent)
    case 'b': {
      std::string arg = optarg;
      if (arg.size() < 2) {
        throw std::runtime_error("argument for -b must look like 10n or 10g");
      }
      char suffix = arg.back();
      std::string num = arg.substr(0, arg.size() - 1);
      size_t pos = 0;
      double v = 0.0;
      try {
        v = std::stod(num, &pos);
      } catch (const std::exception &) {
        pos = 0;
      }
      if (pos == 0 || pos != num.size() || !(v > 0.0)) {
        throw std::runtime_error("argument for -b needs a positive number");
      }
      if (suffix == 'n' || suffix == 'q') {
        if (v != std::floor(v) || v > 1e18) {
          throw std::runtime_error(std::string("-b<N>") + suffix +
                                   " requires a whole number N");
        }
        a.bestKind = suffix == 'n' ? BestKind::Count : BestKind::PerQuery;
      } else if (suffix == 'g') {
        if (v > 100.0) {
          throw std::runtime_error("-b<P>g requires 0 < P <= 100");
        }
        a.bestKind = BestKind::Percent;
      } else {
        throw std::runtime_error("-b must end in 'n' (count), 'g' (percent) or 'q' (per query)");
      }
      a.bestValue = v;
      break;
    }
    case 'o': {
      a.matrixOutfile = optarg;
      break;
    }
    case 's': {
      a.doSine = true;
      break;
    }
    case 'd': {
      a.doDump = true;
      break;
    }
    case 'a': {
      a.doAllByAll = true;
      break;
    }
    case 'I': {
      a.doInterpolation = true;
      break;
    }
    case 'r': {
      a.useRBins = true;
      break;
    }
    case 'l': {
      a.useLogBins = true;
      break;
    }
    case ':': {
      throw std::runtime_error(std::string("option requires an argument -") +
                               static_cast<char>(optopt));
      break;
    }
    case '?': {
      throw std::runtime_error(std::string("option is invalid -") +
                               static_cast<char>(optopt));
      break;
    }
    }
  }

  if (a.mode == option_t::Query && !optIProvided) {
    throw std::runtime_error(
        "The -q option requires -i to specify query and target datasets.");
  } else if (a.mode == option_t::GenerateScoringMatrix && !optIProvided) {
    throw std::runtime_error(
        "The -g option requires -i to specify query and target datasets.");
  } else if (a.mode != option_t::Query && a.doAllByAll) {
    throw std::runtime_error("The -a flag is for QUERY mode");
  }
  if (a.bestKind != BestKind::None && a.mode != option_t::Query) {
    throw std::runtime_error("The -b option is for QUERY mode");
  }
  if (a.bestKind == BestKind::PerQuery && !a.doAllByAll) {
    throw std::runtime_error("The -b<K>q option requires -a");
  }
  for (int i = optind; i < argc; ++i) {
    a.positionalArgs.push_back(argv[i]);
  }

  return a;
}
