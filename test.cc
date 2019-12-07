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

const int kMapWidth = 64;
const int kMapHeight = 64;
const int kVisibleRange = 4;

void enter_callback(int me, int other) {
  Log("[unit(%d)] Say: unit(%d) Enter to my range\n", me, other);
}

void leave_callback(int me, int other) {
  Log("[unit(%d)] Say: unit(%d) Leave from my range\n", me, other);
}

template <class AOIImpl>
void AOIUsage() {
  AOIImpl aoi(kMapWidth, kMapHeight, kVisibleRange, enter_callback, leave_callback);
  aoi.AddUnit(2, 8, 10);
  aoi.AddUnit(4, 9, 1);

  aoi.UpdateUnit(2, 1, 12);
  aoi.UpdateUnit(4, 2, 7);

  aoi.AddUnit(1, 1, 1);
  aoi.AddUnit(2, 2, 2);
  aoi.AddUnit(3, 3, 3);
  aoi.UpdateUnit(1, 60, 60);
  aoi.UpdateUnit(1, 1, 1);
  aoi.RemoveUnit(1);
}

template <class AOIImpl>
void TestAOI(int max_units, const std::string& name, float addSeq[],
             float updateSeq[]) {
  AOIImpl aoi(kMapWidth, kMapHeight, kVisibleRange, [](int, int) {},
              [](int, int) {});
  auto t1 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    // Log("AddUnit, %d,%f,%f\n", i, addSeq[i], addSeq[i + 1]);
    aoi.AddUnit(i, addSeq[i], addSeq[i + 1]);
  }
  auto t2 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    // Log("UpdateUnit, %d,%f,%f\n", i, updateSeq[i], updateSeq[i + 1]);
    aoi.UpdateUnit(i, updateSeq[i], updateSeq[i + 1]);
  }
  auto t3 = std::chrono::steady_clock::now();
  for (int i = 0; i < max_units; ++i) {
    // Log("RemoveUnit,%d\n", i);
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

  Log("%s", "CrosslinkAOI Usage:\n");
  AOIUsage<CrosslinkAOI>();
  Log("%s", "QuadTreeAOI Usage:\n");

  for (int size = 1000; size <= 10000; size += 1000) {
    float addSeq[size * 2];
    float updateSeq[size * 2];
    for (int i = 0; i < size * 2; i += 2) {
      addSeq[i] = rand() % kMapWidth;
      addSeq[i + 1] = rand() % kMapHeight;
      updateSeq[i] = rand() % kMapWidth;
      updateSeq[i + 1] = rand() % kMapHeight;
    }

    TestAOI<CrosslinkAOI>(size, "CrosslinkAOI", addSeq, updateSeq);
    TestAOI<QuadTreeAOI>(size, "QuadTreeAOI", addSeq, updateSeq);
  }

  return 0;
}
