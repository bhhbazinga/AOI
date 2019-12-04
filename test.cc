#include "crosslink_aoi/crosslink_aoi.h"
#include "quadtree_aoi/quadtree_aoi.h"

#include <chrono>
#include <vector>

#include <iostream>
#include <memory>
#include <string>

#define Log(fmt, ...)                  \
  do {                                 \
    fprintf(stderr, fmt, __VA_ARGS__); \
  } while (0)

const int kMapWidth = 50;
const int kMapHeight = 50;
const int kVisibleRange = 5;

void enter_callback(int me, int other) {
  Log("[unit(%d)] Say: unit(%d) Enter to my range\n", me, other);
}

void leave_callback(int me, int other) {
  Log("[unit(%d)] Say: unit(%d) Leave from my range\n", me, other);
}

void CrosslinkAOIUsage() {
  CrosslinkAOI cl_aoi(64, 64, 4, enter_callback, leave_callback);
  cl_aoi.AddUnit(1, 1, 1);
  cl_aoi.AddUnit(2, 2, 2);
  cl_aoi.AddUnit(3, 3, 3);
  cl_aoi.UpdateUnit(1, 60, 60);
  cl_aoi.UpdateUnit(1, 4, 4);
  cl_aoi.RemoveUnit(1);
}

void QuadTreeAOIUsage() {
  QuadTreeAOI qt_aoi(64, 64, 4, enter_callback, leave_callback);
}

void TestCrosslinkAOI(const int max_units) {
  CrosslinkAOI cl_aoi(kMapWidth, kMapHeight, kVisibleRange, [](int, int) {},
                      [](int, int) {});
  auto t1 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    cl_aoi.AddUnit(i, random() % kMapWidth, random() % kMapHeight);
  }
  auto t2 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    cl_aoi.UpdateUnit(i, random() % kMapWidth, random() % kMapHeight);
  }
  auto t3 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    cl_aoi.RemoveUnit(i);
  }
  auto t4 = std::chrono::steady_clock::now();

  Log("[CrosslinkAOI]:%d unit add,timespan=%ldms\n", max_units,
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
  Log("[CrosslinkAOI]:%d unit update,timespan=%ldms\n", max_units,
      std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count());
  Log("[CrosslinkAOI]:%d unit remove,timespan=%ldms\n", max_units,
      std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count());
  Log("%s\n",
      "----------------------------------------------------------------------");
}

int main(int argc, char const* argv[]) {
  (void)argc;
  (void)argv;

  CrosslinkAOIUsage();
  for (int i = 1000; i <= 10000; i += 1000) {
    TestCrosslinkAOI(i);
  }

  return 0;
}
