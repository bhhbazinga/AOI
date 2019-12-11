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

template <class AOIImpl>
void AOIUsage() {
  AOIImpl aoi(64, 64, 10, enter_callback, leave_callback);
  aoi.AddUnit(1, 1, 1);
  aoi.AddUnit(2, 1, 1);
  // aoi.AddUnit(3, 3, 3);
  aoi.UpdateUnit(2, 60, 60);
  // aoi.UpdateUnit(1, 2, 2);
  // aoi.RemoveUnit(1);
}

void QuadTreeAOIUsage() {
  QuadTreeAOI qt_aoi(64, 64, 4, enter_callback, leave_callback);
}

template <class AOIImpl>
void TestAOI(int max_units, const std::string& name) {
  AOIImpl aoi(kMapWidth, kMapHeight, kVisibleRange, [](int, int) {},
              [](int, int) {});
  auto t1 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    aoi.AddUnit(i, random() % kMapWidth, random() % kMapHeight);
  }
  auto t2 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    aoi.UpdateUnit(i, random() % kMapWidth, random() % kMapHeight);
  }
  auto t3 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    aoi.RemoveUnit(i);
  }
  auto t4 = std::chrono::steady_clock::now();

  Log("[%s]:%d unit add,timespan=%ldms\n", name.c_str(), max_units,
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
  Log("[%s]:%d unit update,timespan=%ldms\n", name.c_str(), max_units,
      std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count());
  Log("[%s]:%d unit remove,timespan=%ldms\n", name.c_str(), max_units,
      std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count());
  Log("%s\n",
      "----------------------------------------------------------------------");
}

int main(int argc, char const* argv[]) {
  (void)argc;
  (void)argv;

  // AOIUsage<CrosslinkAOI>();
  AOIUsage<QuadTreeAOI>();
  for (int i = 1000; i <= 1000; i += 1000) {
    // TestAOI<CrosslinkAOI>(i, "CrosslinkAOI");
    // TestAOI<QuadTreeAOI>(i, "QuadTreeAOI");
  }
  return 0;
}
