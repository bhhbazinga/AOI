#include <cmath>

#include "tower_aoi/tower_aoi.h"

const int TowerAOI::near_[][2] = {{1, -1}, {1, 0},   {1, 1},  {0, -1}, {0, 0},
                                  {0, 1},  {-1, -1}, {-1, 0}, {-1, 1}};

struct TowerAOI::Tower {
  AOI::UnitSet unit_set;  // units in same grid
};

TowerAOI::TowerAOI(float width, float height, float visible_range,
                   const AOI::Callback& enter_callback,
                   const AOI::Callback& leave_callback)
    : AOI(width, height, visible_range, enter_callback, leave_callback),
      grid_size_(visible_range / 3.0f),
      rows_(height / grid_size_),
      cols_(width / grid_size_) {
  towers_ = new Tower*[rows_];
  for (int i = 0; i < rows_; ++i) {
    towers_[i] = new Tower();
  }
}

TowerAOI::~TowerAOI() {}

void TowerAOI::AddUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);
  AOI::Unit* unit = NewUnit(id, x, y);
  OnAddUnit(unit);
}

void TowerAOI::UpdateUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);
  AOI::Unit* unit = get_unit(id);
  unit->x = x;
  unit->y = x;
  OnUpdateUnit(unit);
}

void TowerAOI::RemoveUnit(UnitID id) {
  AOI::Unit* unit = get_unit(id);
  OnRemoveUnit(unit);
}

AOI::UnitSet TowerAOI::FindNearbyUnit(const AOI::Unit* unit,
                                      float range) const {
  AOI::UnitSet res_set;
  ForeachNeadbyTower(unit, [&res_set](const Tower* tower) {
    for (auto other : tower->unit_set) {
      res_set.insert(other);
    }
  });
  res_set.erase(const_cast<AOI::Unit*>(unit));
  return res_set;
}

void TowerAOI::ForeachNeadbyTower(
    const AOI::Unit* unit, std::function<void(const Tower*)> func) const {
  int row = ceil(unit->y / grid_size_);
  int col = ceil(unit->x / grid_size_);
  for (int i = 0; i < 9; ++i) {
    int r = row + near_[i][0];
    int c = col + near_[i][1];
    if (r >= 0 && c >= 0) {
      func(&towers_[r][c]);
    }
  }
}

AOI::Unit* TowerAOI::NewUnit(UnitID id, float x, float y) {
  return new AOI::Unit(id, x, y);
}

void TowerAOI::DeleteUnit(AOI::Unit* unit) { delete unit; }