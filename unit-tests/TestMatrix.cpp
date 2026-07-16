#include "Test.hpp"
#include "core/Matrix.hpp"
#include "io/FileIO.hpp"
#include <fstream>
#include <sstream>

#include <cstdio>
#include <unistd.h>

TEST_CASE(test_Matrix_loadMatrixFromTSV) {
  // create a temporary TSV file
  char filename[] = "/tmp/test-matrix-XXXXXX";
  int fd = mkstemp(filename);
  if (fd == -1) {
    perror("mkstemp");
    throw std::runtime_error("Failed to create temp file");
  }
  close(fd);
  std::ofstream file(filename);
  file << "\tcos_0.1\tcos_0.2\tcos_0.3\n";
  file << "1\t1\t2\t3\n";
  file << "2\t4\t5\t6\n";
  file.close();

  Matrix m = loadMatrixFromTSV(filename);

  // check bins
  REQUIRE_EQ(m.getDistanceBins().size(), static_cast<size_t>(2));
  REQUIRE_EQ(m.getAngleBins().size(), static_cast<size_t>(3));

  // check table contents
  REQUIRE_EQ(m.getTable()[0][0], 1);
  REQUIRE_EQ(m.getTable()[0][1], 2);
  REQUIRE_EQ(m.getTable()[1][2], 6);
}

TEST_CASE(test_Matrix_increment_and_score) {
  Matrix m;
  m.getDistanceBins() = {1, 2};
  m.getAngleBins() = {0.1, 0.2};
  m.getTable() = {{0, 0}, {0, 0}};

  m.increment(0.5, 0.05);
  REQUIRE_EQ(m.score(0.5, 0.05), 1);

  m.increment(2.0, 0.2, 3.0);
  REQUIRE_EQ(m.score(2.0, 0.2), 3);
}

TEST_CASE(test_Matrix_prefixSum) {
  Matrix m;
  m.getDistanceBins() = {1, 2};
  m.getAngleBins() = {0.1, 0.2};
  m.getTable() = {{1, 2}, {3, 4}};

  m.prefixSum();

  // 2D prefix sum: table[i][j] = sum of all original values in rows [0..i], cols [0..j]
  // table[0][0] = 1
  // table[0][1] = 1+2 = 3
  // table[1][0] = 1+3 = 4
  // table[1][1] = 1+2+3+4 = 10
  REQUIRE_EQ(m.getTable()[0][0], 1);
  REQUIRE_EQ(m.getTable()[0][1], 3);
  REQUIRE_EQ(m.getTable()[1][0], 4);
  REQUIRE_EQ(m.getTable()[1][1], 10);
}

TEST_CASE(test_Matrix_output_operator) {
  Matrix m;
  m.getDistanceBins() = {1};
  m.getAngleBins() = {0.1};
  m.getTable() = {{42}};

  std::stringstream ss;
  ss << m;

  REQUIRE(ss.str().find("42") != std::string::npos);
}
