#ifndef CROSSLINK_AOI_H
#define CROSSLINK_AOI_H

#include "aoi.h"
#include "crosslink_aoi/crosslink_aoi.h"

// The cross-link model is optimized using skiplist
class CrosslinkAOI : public AOI {
 private:
  class SkipList;
  struct Unit;

 public:
  CrosslinkAOI(float width, float height, float visible_range,
               const AOI::Callback& enter_callback,
               const AOI::Callback& leave_callback);

  ~CrosslinkAOI() override;

  void AddUnit(UnitID id, float x, float y) override;
  void UpdateUnit(UnitID id, float x, float y) override;
  void RemoveUnit(UnitID id) override;

 protected:
  AOI::UnitSet FindNearbyUnit(const AOI::Unit* unit,
                              float range) const override;

 private:
  AOI::Unit* NewUnit(UnitID id, float x, float y) override;
  void DeleteUnit(AOI::Unit* unit) override;

  SkipList* x_list_;
  SkipList* y_list_;
};
#endif  // CROSSLINK_AOI_H