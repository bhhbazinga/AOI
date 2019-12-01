#include "crosslink_aoi.h"

AOI::Unit* CrosslinkAOI::AddUnit(int id, float x, float y,
                                 AOI::Callback enter_callback,
                                 AOI::Callback leave_callback) {
  AOI::Unit* unit =
      AOI::AddUnit(id, x, y, enter_callback, leave_callback);
  x_list_.Insert(unit);
  y_list_.Insert(unit);

  AOI::UnitSet enter_set = FindNearbyUnit(unit, visible_range_);
  NotifyEnter(unit, enter_set);
  unit->subscribe_set = std::move(enter_set);
  
  return unit;
}

void CrosslinkAOI::UpdateUnit(int id, float x, float y) {
  AOI::UpdateUnit(id, x, y);
  AOI::Unit* unit = get_unit(id);
  const AOI::UnitSet& old_set = unit->subscribe_set;
  x_list_.Erase(unit);
  y_list_.Erase(unit);
  unit->x = x;
  unit->y = y;
  x_list_.Insert(unit);
  y_list_.Insert(unit);

  AOI::UnitSet new_set = FindNearbyUnit(unit, visible_range_);
  AOI::UnitSet move_set = Intersection(old_set, new_set);
  AOI::UnitSet enter_set = Difference(new_set, move_set);
  AOI::UnitSet leave_set = Difference(old_set, new_set);
  unit->subscribe_set = std::move(new_set);

  NotifyAll(unit, enter_set, leave_set);
}

void CrosslinkAOI::RemoveUnit(int id) {
  AOI::Unit* unit = get_unit(id);
  const AOI::UnitSet& subscribe_set = unit->subscribe_set;
  NotifyLeave(unit, subscribe_set);

  AOI::RemoveUnit(id);
}

AOI::UnitSet CrosslinkAOI::FindNearbyUnit(AOI::Unit* unit, const float range) const {
  // Used to range query
  Unit lower_x_unit(0, unit->x - range, 0);
  Unit upper_x_unit(0, unit->x + range, 0);
  auto lower_x_it = x_list_.FindFirstGreater(&lower_x_unit);
  auto upper_x_it = x_list_.FindLastLess(&upper_x_unit);
  ++upper_x_it;
  AOI::UnitSet x_set;
  for (auto it = lower_x_it; it != upper_x_it; ++it) {
    x_set.insert(*it);
  }

  // Used to range query
  Unit lower_y_unit(0, unit->y - range, 0);
  Unit upper_y_unit(0, unit->y + range, 0);
  auto lower_y_it = x_list_.FindFirstGreater(&lower_y_unit);
  auto upper_y_it = x_list_.FindLastLess(&upper_y_unit);
  ++upper_y_it;
  AOI::UnitSet res_set;
  for (auto it = lower_y_it; it != upper_y_it; ++it) {
    if (x_set.find(*it) != x_set.end()) {
      res_set.insert((*it));
    }
  }

  // Exclude unit itself
  res_set.erase(unit);
  return res_set;
}
