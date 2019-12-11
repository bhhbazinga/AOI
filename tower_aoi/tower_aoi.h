
#ifndef TOWER_AOI_H
#define TOWER_AOI_H

#include "aoi.h"

class TowerAOI : public AOI {
 private:
  struct Tower;

 public:
  TowerAOI(float width, float height, float visible_range,
           const AOI::Callback& enter_callback,
           const AOI::Callback& leave_callback);
  ~TowerAOI() override;

  void AddUnit(UnitID id, float x, float y) override;
  void UpdateUnit(UnitID id, float x, float y) override;
  void RemoveUnit(UnitID id) override;

 protected:
  AOI::UnitSet FindNearbyUnit(const AOI::Unit* unit,
                              float range) const override;

 private:
  AOI::Unit* NewUnit(UnitID id, float x, float y) override;
  void DeleteUnit(AOI::Unit* unit) override;
  void ForeachNeadbyTower(const AOI::Unit* unit,
                          std::function<void(const Tower*)>) const;

  const float grid_size_;
  const int cols_;
  const int rows_;
  Tower** towers_;
  static const int near_[][2];
};

#endif  // TOWER_AOI_H