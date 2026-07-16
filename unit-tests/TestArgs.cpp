#include "Test.hpp"
#include "utils/ArgParse.hpp"

#include <vector>
#include <unistd.h>
#include <sys/wait.h>

extern int optind;

// ==================== helpers ====================
static std::vector<char*> make_argv(std::vector<const char*> args) {
    std::vector<char*> result;
    for (auto s : args)
        result.push_back(const_cast<char*>(s));
    result.push_back(nullptr);
    return result;
}

// ==================== tests ====================

TEST_CASE(test_optToString_query) {
    REQUIRE_EQ(optToString(option_t::Query), "q");
    REQUIRE_EQ(optToString(option_t::GenerateScoringMatrix), "g");
    REQUIRE_EQ(optToString(option_t::DefaultMode), "default");
}

TEST_CASE(test_args_parse_query_mode) {
    optind = 1;
    Args args;

    auto argv = make_argv({
        "./prog",
        "-q",
        "matrix.tsv",
        "-i",
        "/tmp/test1,/tmp/test2"
    });

    int argc = argv.size() - 1;
    args = parseArgs(argc, argv.data());

    REQUIRE_EQ(args.mode, option_t::Query);
    REQUIRE_EQ(args.matrixFilepath, "matrix.tsv");
    REQUIRE_EQ(args.queryDatasetFilepath, "/tmp/test1");
    REQUIRE_EQ(args.targetDatasetFilepath, "/tmp/test2");
}

TEST_CASE(test_args_parse_generate_mode) {
    optind = 1;
    Args args;

    auto argv = make_argv({
        "prog",
        "-g",
        "known.tsv,123",
        "-i",
        "/tmp/test1,/tmp/test2"
    });

    int argc = argv.size() - 1;

    args = parseArgs(argc, argv.data());

    REQUIRE_EQ(args.mode, option_t::GenerateScoringMatrix);
    REQUIRE_EQ(args.knownMatchesFilepath, "known.tsv");
    REQUIRE_EQ(args.numGeneratorIterations, 123u);
    REQUIRE_EQ(args.queryDatasetFilepath, "/tmp/test1");
    REQUIRE_EQ(args.targetDatasetFilepath, "/tmp/test2");
}

TEST_CASE(test_args_parse_sine_flag) {
    optind = 1;
    Args args;

    auto argv = make_argv({
        "prog",
        "-s",
        "-q",
        "matrix.tsv",
        "-i",
        "/tmp/test1,/tmp/test2"
    });

    int argc = argv.size() - 1;

    args = parseArgs(argc, argv.data());

    REQUIRE(args.doSine);
}
