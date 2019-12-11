
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
  void CalculateRowCol(const AOI::Unit* unit, int* row, int* col) const;

  const int rows_;
  const int cols_;
  Tower** towers_;
};

#endif  // TOWER_AOI_H