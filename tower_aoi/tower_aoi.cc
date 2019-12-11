#include <cmath>

#include "tower_aoi/tower_aoi.h"

struct TowerAOI::Tower {
  AOI::UnitSet unit_set;  // units in same grid
};

TowerAOI::TowerAOI(float width, float height, float visible_range,
                   const AOI::Callback& enter_callback,
                   const AOI::Callback& leave_callback)
    : AOI(width, height, visible_range, enter_callback, leave_callback),
      rows_(ceil(height / visible_range)),
      cols_(ceil(width / visible_range)) {
  towers_ = new Tower*[rows_];
  for (int i = 0; i < rows_; ++i) {
    towers_[i] = new Tower[cols_];
  }
}

TowerAOI::~TowerAOI() {
  std::vector<UnitID> unit_ids = get_unit_ids();
  for (auto id : unit_ids) {
    RemoveUnit(id);
  }

  for (int i = 0; i < rows_; ++i) {
    delete[] towers_[i];
  }
  delete[] towers_;
}

void TowerAOI::AddUnit(UnitID id, float x, float y) {
  ValidatetUnitID(id);
  ValidatePosition(x, y);

  AOI::Unit* unit = NewUnit(id, x, y);
  int row, col;
  CalculateRowCol(unit, &row, &col);
  towers_[row][col].unit_set.insert(unit);

  OnAddUnit(unit);
}

void TowerAOI::UpdateUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);
  AOI::Unit* unit = get_unit(id);

  int old_row, old_col, new_row, new_col;
  CalculateRowCol(unit, &old_row, &old_col);
  unit->x = x;
  unit->y = y;
  CalculateRowCol(unit, &new_row, &new_col);
  if (old_row == new_row && old_col == new_col) {
    return;
  }
  towers_[old_row][old_col].unit_set.erase(unit);
  towers_[new_row][new_col].unit_set.insert(unit);

  OnUpdateUnit(unit);
}

void TowerAOI::RemoveUnit(UnitID id) {
  AOI::Unit* unit = get_unit(id);
  int row, col;
  CalculateRowCol(unit, &row, &col);
  towers_[row][col].unit_set.erase(unit);
  OnRemoveUnit(unit);
}

inline void TowerAOI::CalculateRowCol(const AOI::Unit* unit, int* row,
                                      int* col) const {
  float visible_range = get_visible_range();
  *row = std::clamp(static_cast<int>(floor(unit->y / visible_range)), 0,
                    rows_ - 1);
  *col = std::clamp(static_cast<int>(floor(unit->x / visible_range)), 0,
                    cols_ - 1);
}

AOI::UnitSet TowerAOI::FindNearbyUnit(const AOI::Unit* unit,
                                      float range) const {
  int row, col;
  CalculateRowCol(unit, &row, &col);
  int span = ceil(range / get_visible_range());
  int start_row = std::max(row - span, 0);
  int start_col = std::max(col - span, 0);
  int end_row = std::min(row + span, rows_ - 1);
  int end_col = std::min(col + span, cols_ - 1);
  AOI::UnitSet res_set;
  for (int i = start_row; i <= end_row; ++i) {
    for (int j = start_col; j <= end_col; ++j) {
      const AOI::UnitSet& unit_set = towers_[i][j].unit_set;
      for (auto other : unit_set) {
        if (fabs(unit->x - other->x) <= range &&
            fabs(unit->y - other->y) <= range) {
          res_set.insert(other);
        }
      }
    }
  }
  res_set.erase(const_cast<AOI::Unit*>(unit));
  return res_set;
}

AOI::Unit* TowerAOI::NewUnit(UnitID id, float x, float y) {
  return new AOI::Unit(id, x, y);
}

void TowerAOI::DeleteUnit(AOI::Unit* unit) { delete unit; }