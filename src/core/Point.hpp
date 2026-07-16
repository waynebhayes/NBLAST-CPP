#ifndef POINT_HPP
#define POINT_HPP

#include "Matrix.hpp"
#include <iostream>
#include <string>
#include <vector>

using DoubleVector = std::vector<double>;
using DoubleVector2D = std::vector<DoubleVector>;

constexpr int POINT_DEFAULT_PARENT = -1;
constexpr int POINT_DEFAULT_ID = -1;

// Individual points, stores parent, position, etc.
struct Point {
  int id;
  double x, y, z;
  int parent;
  double tx, ty, tz; // tangent vector

  Point(int id, double x, double y, double z, int parent, double tx = 0,
        double ty = 0, double tz = 0)
      : id(id), x(x), y(y), z(z), parent(parent), tx(tx), ty(ty), tz(tz) {}
  Point()
      : id(POINT_DEFAULT_ID), x(0.0), y(0.0), z(0.0),
        parent(POINT_DEFAULT_PARENT), tx(0), ty(0), tz(0) {}

  void parse(std::string line);

  double magnitude(void) const;

  Point midpoint(const Point &other) const;
  double distance(const Point &other) const;
  double angleMeasure(const Point &other, bool do_cosine) const;

  friend double dotProduct(const Point &lhs, const Point &rhs);
  friend std::ostream &operator<<(std::ostream &out, const Point &p);
  friend Point operator-(const Point &lhs, const Point &rhs);
  friend Point operator+(const Point &lhs, const Point &rhs);
  friend Point operator*(const Point &lhs, double rhs);
  friend Point operator*(double lhs, const Point &rhs);
};
using PointVector = std::vector<Point>;

// Alignment structure, stores point-ids, distance, etc.
struct PointAlignment {
  int queryPointID, targetPointID;
  double distance, angleMeasure, score;

  PointAlignment(int queryPointID, int targetPointID, double distance,
                 double angleMeasure)
      : queryPointID(queryPointID), targetPointID(targetPointID),
        distance(distance), angleMeasure(angleMeasure), score(0) {}
  PointAlignment()
      : queryPointID(-1), targetPointID(-1), distance(0), angleMeasure(0),
        score(0) {}
  PointAlignment(const PointAlignment &other)
      : queryPointID(other.queryPointID), targetPointID(other.targetPointID),
        distance(other.distance), angleMeasure(other.angleMeasure),
        score(other.score) {}

  void computeRawScore(const Matrix &mat);

  void printDifference(std::ostream &out, const std::string &tag = "") const;
  void printScore(std::ostream &out) const;
  PointAlignment operator=(const PointAlignment &other);
  bool operator<(const PointAlignment &other) const {
    return distance < other.distance;
  }

  friend PointAlignment operator+(const PointAlignment &lhs,
                                  const PointAlignment &rhs);
};
using PAVector = std::vector<PointAlignment>;

// KD-tree cloud (const)
struct PointCloud {
  const PointVector &pts;

  PointCloud(const PointVector &points) : pts(points) {}

  inline size_t kdtree_get_point_count() const { return pts.size(); }

  inline double kdtree_get_pt(size_t idx, size_t dim) const {
    const Point &p = pts[idx];
    if (dim == 0)
      return p.x;
    if (dim == 1)
      return p.y;
    return p.z;
  }

  // bounding-box (not used)
  template <class BBOX> bool kdtree_get_bbox(BBOX &) const { return false; }
};

#endif // POINT_HPP
